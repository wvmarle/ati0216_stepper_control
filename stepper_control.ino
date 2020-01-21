/*
  Stepper controller.

  Encoder w/push button as input.

  Red/green indicator LED.

  Upon power up:
    set process state to HOMING.


   Normal running:
      HOMING
        LED blinks red
        Motor finds home position.
        switch LED to green - machine ready to use.

      ENTER_TIME
        LED continuous green.u
        Motor stopped.
        Display shows current set time.
        Set time with encoder: encoder one click per second increase or decrease.
        If encoder pressed: MOVE_DOWN.

      MOVE_UP
        LED blinks red.
        Motor moves down by some preset distance.
        Display shows current set time.
        If encoder pressed: INTERRUPTED.
        When distance reached: COUNTDOWN.

      COUNTDOWN
        LED continuous red.
        Motor stopped.
        Display shows time left - counting down to zero.
        Encoder input is ignored.
        When time 00:00: MOVE_UP.

      MOVE_DOWN
        LED blinks red.
        Motor moves up by some preset distance, opposite direction.
        Display shows 00:00.
        Encoder input is ignored.
        When distance reached: COMPLETED.

      COMPLETED
        LED blinks green.
        Motor stopped.
        Display shows original set time.
        If encoder pressed: MOVE_DOWN.
        If encoder turned: ENTER_TIME.



  TODO:
  - find home switch every time when moving up.
  - check for home switch while moving up: stop when the switch is triggered.
  - check: home switch not within 100 full steps from where it should be when moving up.
  - check: home switch not found within full movement + 500 full steps (20 mm).
  - display errors:
    Err1 = home switch not found on initial homing.
    Err2 = home switch triggered too early (lost steps on the way down).
    Err3 = home switch triggered too late (lost steps on the way up).

*/
#include <Encoder.h>
#include <TM1637.h>
#include <EEPROM.h>

#define DEMO_MODE

#ifdef DEMO_MODE
#pragma message("Compiling for DEMO MODE.")
#endif

/*******************************************************************************
   System defaults - change these as needed.

   Note: beware to not exceed technical limits.
      Stepper maximum speed: 600 rpm.

      MCU maximum speed: not determined, estimate 550 rpm.

 *******************************************************************************/
#ifdef DEMO_MODE
const int16_t MIN_TIME = 10;                                // Minimum allowed time: 10 seconds.
const int16_t MAX_TIME = 50;                                // Maximum allowed time: 45 seconds.
#else
const int16_t MIN_TIME = 10;                                // Minimum allowed time: 10 seconds.
const int16_t MAX_TIME = 5990;                              // Maximum allowed time: 99 minutes, 50 seconds.
#endif
const float NORMAL_BLINK_FREQUENCY = 1;                     // Speed (in Hz) of the normal blinking of the LED and display.
const float FAST_BLINK_FREQUENCY = 1.5;                     // Speed (in Hz) of the fast blinking of the LED and display.
const int32_t MOVE_DISTANCE = 150;                          // Distance of the required basket movement in mm.
const int32_t HOMING_RPM = 30;                              // The desired speed for homing.
const int32_t MOVE_RPM = 200;                               // The desired speed for normal up/down movement.
const int32_t SHAKE_RPM = 380;                              // The desired speed for shaking up stuff.
const uint8_t TIME_STEP = 10;                               // Seconds per encoder tick to add/subtract.
const float ACCEL_TIME = 1.5;                               // The time in seconds to take for accelleration/decelleration.
const uint32_t PASTA_SHAKE_INTERVAL = 5;                    // Interval in seconds between two "shakes" in PASTA mode.
const float SHAKE_ACCEL_TIME = 0.2;                         // The time in seconds to take for accelleration/decelleration when doing a shake.
const uint16_t SHAKE_DISTANCE = 10;                         // Distance the basket moves up and down when doing a shake.
const uint32_t INTERRUPT_TIME = 3000;                       // Time in milliseconds the button has to be pressed to interrupt the cooking countdown.

/*******************************************************************************
    End user adjustable settings.
 *******************************************************************************/

// The LED colours - dual LED connected to one pin; set to INPUT for all off.
#define GREEN HIGH
#define RED LOW
#define BICOLOUR 2                                          // Special colour for blinking: alternate between the two colours.

// The directions of the stepper.
#define DOWN HIGH
#define UP LOW

// The state of the cooking mode switch.
#define PASTA HIGH
#define EGGS LOW

/*******************************************************************************
   Pin definitions.
 *******************************************************************************/
const uint8_t DIO = 3;    // PA3
const uint8_t CLK = 4;    // PA4
const uint8_t STEP = 6;   // PA6
const uint8_t DIR = 1;    // PA1
const uint8_t ENN = 2;    // PA2
const uint8_t LED = 7;    // PA7
const uint8_t ENCA = 10;  // PB0
const uint8_t ENCB = 9;   // PB1
const uint8_t ENCS = 8;   // PB2
const uint8_t HOME = 0;   // PA0
const uint8_t MODE = 5;   // PA5

/*******************************************************************************
   Other system settings.
 *******************************************************************************/
const uint32_t CLOCKSPEED = 8000000;                        // CPU clock speed in Hz.
const uint32_t STEPS_PER_ROTATION = 200;
const uint32_t MICROSTEPS = 32;
const uint32_t DISTANCE_PER_ROTATION = 8;

// Calculated system constants.
const uint32_t STEPPER_MOVE_STEPS = 2 * STEPS_PER_ROTATION * MICROSTEPS * MOVE_DISTANCE / DISTANCE_PER_ROTATION; // Note: steps are counted double as we count edges!
const uint32_t STEPPER_MOVE_SPEED = STEPS_PER_ROTATION * MICROSTEPS * MOVE_RPM / 60;    // Steps per second.
const uint32_t STEPPER_SHAKE_STEPS = 2 * STEPS_PER_ROTATION * MICROSTEPS * SHAKE_DISTANCE / DISTANCE_PER_ROTATION; // Note: steps are counted double as we count edges!
const uint32_t STEPPER_SHAKE_SPEED = STEPS_PER_ROTATION * MICROSTEPS * SHAKE_RPM / 60;  // Steps per second.
const uint16_t HOMING_SPEED = STEPS_PER_ROTATION * MICROSTEPS * HOMING_RPM / 60;        // Steps per second.
const uint32_t STEPPER_MINIMUM_SPEED = 0.05 * STEPPER_MOVE_SPEED;
const uint32_t STEPPER_SHAKE_MINIMUM_SPEED = 0.05 * STEPPER_SHAKE_SPEED;
const float INTERRUPTS_PER_SECOND = CLOCKSPEED / ((float)256 * 256);
const uint32_t ACCEL_STEPS = ACCEL_TIME * STEPPER_MOVE_SPEED + 0.5 * ACCEL_TIME * STEPPER_MINIMUM_SPEED; // Number of steps the acceleration/deceleration takes.
const uint32_t ACCEL_SPEED_STEP = float(STEPPER_MOVE_SPEED - STEPPER_MINIMUM_SPEED) / (ACCEL_TIME * INTERRUPTS_PER_SECOND);
const uint32_t SHAKE_ACCEL_STEPS = SHAKE_ACCEL_TIME * STEPPER_SHAKE_SPEED + 0.5 * SHAKE_ACCEL_TIME * STEPPER_SHAKE_MINIMUM_SPEED;
const uint32_t SHAKE_ACCEL_SPEED_STEP = float(STEPPER_SHAKE_SPEED - STEPPER_SHAKE_MINIMUM_SPEED) / (SHAKE_ACCEL_TIME * INTERRUPTS_PER_SECOND);
const uint32_t BLINK_SPEED_NORMAL = INTERRUPTS_PER_SECOND / (2.0 * NORMAL_BLINK_FREQUENCY);
const uint32_t BLINK_SPEED_FAST = INTERRUPTS_PER_SECOND / (2.0 * FAST_BLINK_FREQUENCY);
uint32_t blinkSpeed = BLINK_SPEED_NORMAL;

// The various system states.
enum {
  HOMING,                                                   // Stepper is searching it's home.
  ENTER_TIME,                                               // User enters the time.
  MOVE_DOWN,                                                // Cooker moving down.
  COUNTDOWN,                                                // Countdown - cooking in progress.
  MOVE_UP,                                                  // Cooker moving up.
  COMPLETED,                                                // Cooking process completed.
  ERROR_STATE,                                              // Error detected.
} processState = HOMING;                                    // Start by homing the stepper.

/*******************************************************************************
   Global variables.
 *******************************************************************************/
// Timing.
uint32_t countdownStartTime;                                // When the cooking countdown started.
int16_t cookingTime;                                        // The total time we have to cook.
uint32_t lastShake;                                         // Last time we shook the basket (in PASTA mode).

// Display.
uint8_t displayData[] = {8, 8, 8, 8};                       // Set initial display to 88:88
bool clockPoint = POINT_ON;                                 // Whether the point is on or off.
bool displayBlinks;                                         // Whether to blink the display between off and showing cookingTime, in conjunction with the LED.

// Inputs (encoder, home switch, mode switch).
int32_t encoderReading;
int32_t oldEncoderReading;
bool switchState;
bool oldSwitchState;

// Stepper control.
volatile bool movementComplete;                             // Whether the stepper has reached the target position.
volatile int32_t stepperPosition;                           // The current position of the stepper.
volatile int32_t stepperSpeed;                              // The current speed of the stepper, in steps per second.
int16_t stepperSpeedStep;                                   // The step the speed takes while accelerating/decellerating.
uint32_t stepperTargetSpeed;                                // Speed the stepper has to accelerate to.
volatile int32_t targetPosition;                            // Position where the stepper has to move to.
int32_t startDecelerating;                                  // The step at which the deceleration should start.
bool homing;                                                // Whether we're searching for the home switch.
bool cookingMode;                                           // Based on the mode switch: "eggs" (normal) or "pasta" (shaking now and then).
volatile int32_t minimumSpeed;                              // Minimum speed for the stepper.
const uint16_t EEPROM_COOKINGTIME = 0;                      // Store the last used cooking time in EEPROM.
bool stepperDirection;

uint8_t errorCode = 0;
volatile uint8_t LEDColour;
volatile bool blinkState;
volatile uint16_t blinkCounter;

Encoder encoder(ENCA, ENCB);                                // Rotary encoder for changing variables.
TM1637 tm1637(CLK, DIO);                                    // The LED display.

/*******************************************************************************
   setup function.
 *******************************************************************************/
void setup() {
  tm1637.init();
  tm1637.set(2);                                            // Set the brightness of the display.
  updateDisplay();
  pinMode(ENCS, INPUT_PULLUP);
  pinMode(LED, INPUT);                                      // Switch off the LED.
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(HOME, INPUT_PULLUP);
  pinMode(MODE, INPUT);                                     // Home blocking circuit makes this active HIGH, includes pull-down.
  pinMode(ENN, OUTPUT);
  digitalWrite(ENN, LOW);                                   // Enable the stepper.
  EEPROM.get(EEPROM_COOKINGTIME, cookingTime);

  // Set up timer1.
  TCCR1A = 0;
  TCCR1B = 0;
  bitSet(TCCR1B, CS10);                                     // Clock with no prescaler.
  bitSet(TCCR1B, WGM12);                                    // CTC mode (Clear Timer on Compare).

  // Set up timer0. This does mess with the millis() function!
  // Change the default clk/64 to clk/256, making the timer run 4 times slower.
  // This makes for just over 122 ticks per second at our 8 MHz clock speed. We use a variable timer instead of millis() function.
  bitSet(TCCR0B, CS02);
  bitClear(TCCR0B, CS01);
  bitClear(TCCR0B, CS00);
  bitSet(TIMSK0, OCIE0B);                                   // Enable the LED blinker interrupt.
  OCR0B = 127;                                              // Call the interrupt halfway the timer run; spread the load a bit.

  // Start by homing.
  homing = true;                                            // We're homing the stepper now.
  movementComplete = true;                                  // Trigger the next step.
  stepperDirection = UP;                                    // Homing means we're moving up first.
  digitalWrite(DIR, UP);
  digitalWrite(ENN, LOW);                                   // Enable the stepper.
  processState = HOMING;
  blinkLED(BICOLOUR);
}

/*******************************************************************************
   Main loop.
 *******************************************************************************/
void loop() {
  switchState = digitalRead(ENCS);
  encoderReading = encoder.read() >> 2;                     // Use 1/4 the reading: it counts four per click.
  switch (processState) {
    case HOMING:
      handleHoming();
      break;

    case ENTER_TIME:
      cookingMode = digitalRead(MODE);
      handleEnterTime();
      break;

    case COMPLETED:
      handleCompleted();
      break;

    case MOVE_DOWN:
      handleMoveDown();
      break;

    case COUNTDOWN:
      handleCountdown();
      break;

    case MOVE_UP:
      handleMoveUp();
      break;
  }
  if (errorCode > 0) {                                      // An error occurred; show it on the display and stop operations.
    handleError();
  }
  oldSwitchState = switchState;
  oldEncoderReading = encoderReading;
}
/*******************************************************************************
   Millis() correction.
   As we change timer0 settings, we have to correct the millis() value.
   Now myMillis() can be used instead of the normal millis() timer.
 *******************************************************************************/
uint32_t myMillis() {
  return millis() << 2;
}

/*******************************************************************************
   Display handling.
 *******************************************************************************/
void displayTime(uint16_t t) {
  uint8_t minutes = t / 60;
  uint8_t seconds = t % 60;
  displayData[0] = minutes / 10;
  displayData[1] = minutes % 10;
  displayData[2] = seconds / 10;
  displayData[3] = seconds % 10;
  updateDisplay();
}

void displayValue(uint16_t val) {
  displayData[0] = (val / 1000) % 10;
  displayData[1] = (val / 100) % 10;
  displayData[2] = (val / 10) % 10;
  displayData[3] = val % 10;
  clockPoint = POINT_OFF;
  updateDisplay();
}

void updateDisplay() {
  tm1637.point(clockPoint);
  tm1637.display(displayData);
}

/*******************************************************************************
   LED handling.
 *******************************************************************************/
bool LEDBlinks;

void  blinkLED(uint8_t colour) {
  if (colour == BICOLOUR) {
    LEDOn(GREEN);
    LEDBlinks = true;
    blinkState = false;
  }
  else {
    if (LEDBlinks == false) {                               // If we're not blinking at this moment.
      LEDBlinks = true;                                     // start blinking,
      blinkState = false;                                   // start with the off part of the cycle, and
      LEDOff();
      blinkCounter = 0;                                     // restart the counter.
    }
    bitWrite(PORTA, PA7, colour);                           // Set LED to the correct colour right away.
  }
  LEDColour = colour;
}

void setLED(bool colour) {
  LEDBlinks = false;
  LEDOn(colour);
}

void LEDOn(bool colour) {
  bitSet(DDRA, PA7);
  bitWrite(PORTA, PA7, colour);
}

void LEDOff() {
  bitClear(DDRA, PA7);
  bitClear(PORTA, PA7);
}

/*******************************************************************************
   Timer ISRs.
 *******************************************************************************/
/*
  Frequency of calling the timer interrupt vs. rpm (8 MHz clock):
  1 rpm = 213 1/3 steps per second, 37500 ticks per complete step, 18750 ticks per interrupt.
  30 rpm = 6400 Hz, 625 ticks per call.
  150 rpm = 32 kHz, 125 ticks per call.
  500 rpm = 107 kHz, 37.5 ticks per call. Too fast! Have to go down to 32 microsteps.

  Speed in rpm increases linear - so steps per second increases linear.
*/

ISR(TIMER1_COMPA_vect) {
  stepperPosition++;                                        // Moving down.
  PORTA ^= (1 << PA6);
}

ISR(TIMER1_COMPB_vect) {
  stepperPosition--;                                        // Moving up.
  PORTA ^= (1 << PA6);
}

/*
   Prescaler: clk/256
   Overflow: 122.07 times per second, or every 8.192 ms.
*/
ISR(TIMER0_COMPA_vect) {
  stepperSpeed += stepperSpeedStep;                         // Calculate the new speed for the stepper in steps per second.
  if (stepperSpeed > stepperTargetSpeed) {                  // Desired target speed reached!
    TIMSK0 &= ~(1 << OCIE0A);                               // Disable this interrupt, we're done.
    stepperSpeed = stepperTargetSpeed;
  }
  else if (stepperSpeed < minimumSpeed) {                   // Speed dropped below the minimum. Maybe we have steps left, so don't stop completely.
    TIMSK0 &= ~(1 << OCIE0A);                               // Disable this interrupt, we're done.
    stepperSpeed = minimumSpeed;
  }
  OCR1A = CLOCKSPEED / (stepperSpeed * 2.0);               // Set the new stepper speed; the factor 2 for two edges per step.
  OCR1B = OCR1A;
}

// Help out with the blinking of the LED and checking the stepper position.
ISR(TIMER0_COMPB_vect) {
  static bool slowingDown;
  doBlinking();
  if (stepperDirection == DOWN) {
    movementComplete = (stepperPosition >= targetPosition);
  }
  else {
    movementComplete = (stepperPosition <= targetPosition);
  }
  if (movementComplete == false) {
    if (slowingDown == false) {                             // We're not slowing down yet, check if it's time to do so.
      if ((stepperDirection == DOWN && stepperPosition >= startDecelerating) ||
          (stepperDirection == UP && stepperPosition <= startDecelerating)) {
        slowingDown = true;
        stepperSpeedStep = -stepperSpeedStep;               // Slow down - set steps negative.
        TIMSK0 |= (1 << OCIE0A);                            // Enable the acceleration interrupt.
      }
    }
  }
  else {
    bitClear(TIMSK1, OCIE1A);                               // Movement done - switch off this interrupt.
    bitClear(TIMSK1, OCIE1B);                               // Movement done - switch off this interrupt.
    slowingDown = false;                                    // We're also not accelerating or decelerating any more.
  }
}

void doBlinking() {
  if (LEDBlinks) {
    blinkCounter++;
    if (blinkCounter > blinkSpeed) {
      blinkCounter = 0;
      if (blinkState) {
        if (LEDColour == BICOLOUR) {
          LEDOn(GREEN);
        }
        else {
          LEDOff();
        }
        blinkState = false;
      }
      else {
        if (LEDColour == BICOLOUR) {
          LEDOn(RED);
        }
        else {
          LEDOn(LEDColour);
        }
        blinkState = true;
      }
      if (displayBlinks) {                                    // We may be blinking the display as well!
        if (blinkState) {                                     // Display on when LED is on.
          displayTime(cookingTime);
        }
        else {
          tm1637.point(POINT_OFF);
          tm1637.clearDisplay();
        }
      }
    }
  }
}
