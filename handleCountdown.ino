/*
  COUNTDOWN
    LED continuous red.
    Motor stopped.
    Display shows time left - counting down to zero.
    Encoder input is ignored.
    When time 00:00: MOVE_DOWN.
*/


const uint8_t SHAKE_DONE = 1;                               // No shaking going on.
const uint8_t SHAKE_UP = 2;                                 // When "shake up" is done, start "shake down".
const uint8_t SHAKE_DOWN = 3;                               // Nothing to do here, really...
uint8_t shakeState = SHAKE_DONE;

void handleCountdown() {
  static bool oldClockPoint;
  int16_t t = cookingTime * 2 - (myMillis() - countdownStartTime) / 500 ; // Calculate the time left, in half seconds.
  clockPoint = bitRead(t, 0);                               // off if even; on if odd.
  if (t == 0) {                                             // Time's up, time to move up again.
    clockPoint = POINT_ON;
    displayTime(0);                                         // update the display.
    startMoveUp();
    processState = MOVE_UP;
  }
  else if (clockPoint != oldClockPoint) {                   // When the clockPoint has changed,
    displayTime(t >> 1);                                    // update the display - divide by 2 to have the actual seconds.
  }
  oldClockPoint = clockPoint;                               // Remember the old time.
  if (cookingMode == PASTA) {                               // Cooking pasta: shake up things every now and then.
    if (myMillis() - lastShake > (PASTA_SHAKE_INTERVAL * 1000)) { // Time to start a new shake!
      lastShake = myMillis();
      shakeState = SHAKE_UP;
    }
    if (movementComplete) {
      switch (shakeState) {
        case SHAKE_UP:
          stepperDirection = UP;
          targetPosition = stepperPosition - STEPPER_SHAKE_STEPS;
          startShakeMovement();
          shakeState = SHAKE_DOWN;
          break;

        case SHAKE_DOWN:
          stepperDirection = DOWN;
          targetPosition = stepperPosition + STEPPER_SHAKE_STEPS;
          startShakeMovement();
          shakeState = SHAKE_DONE;
      }
    }
  }

#ifndef DEMO_MODE                                           // Ignore the encoder when in demo mode.
  static uint32_t switchPressedTime;
  if (switchState == LOW) {                                 // Switch is just pressed - interrupt the cooking process.
    if (oldSwitchState == HIGH) {                           // It happened just now!
      oldSwitchState = LOW;
      switchPressedTime = myMillis();                       //
    }
    else {
      if (myMillis() - switchPressedTime > INTERRUPT_TIME) {
        startMoveUp();                                      // Start moving up - cancelling the cooking process.
        processState = MOVE_UP;
      }
    }
  }
#endif
}

void startShakeMovement() {
  stepperSpeed = STEPPER_SHAKE_MINIMUM_SPEED;
  stepperTargetSpeed = STEPPER_SHAKE_SPEED;                 // Move at this speed (steps per second).
  stepperSpeedStep = SHAKE_ACCEL_SPEED_STEP;
  minimumSpeed = STEPPER_SHAKE_MINIMUM_SPEED;
  if (targetPosition > stepperPosition) {                   // We're about to start moving DOWN.
    stepperDirection = DOWN;
    startDecelerating = targetPosition - SHAKE_ACCEL_STEPS;
    bitSet(TIMSK1, OCIE1A);                                 // Stepper step interrupt: counts up for going down.
  }
  else {                                                    // We're about to start moving UP.
    stepperDirection = UP;
    startDecelerating = targetPosition + SHAKE_ACCEL_STEPS;
    bitSet(TIMSK1, OCIE1B);                                 // Stepper step interrupt: counts down for going up.
  }
  movementComplete = false;
  digitalWrite(DIR, stepperDirection);
  bitSet(TIMSK0, OCIE0A);                                   // Acceleration interrupt.
}
