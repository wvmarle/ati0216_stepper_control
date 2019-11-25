/*
  MOVE_UP
    LED blinks red.
    Motor moves some preset distance, opposite direction.
    Display shows 00:00.
    Encoder input is ignored.
    When distance reached: COMPLETED.
*/
void handleMoveUp() {
  if (movementComplete) {
    processState = COMPLETED;
    clockPoint = POINT_ON;
    displayTime(cookingTime);
    if (digitalRead(HOME) == HIGH) {                      // Home switch is triggered! When this happens the stepper gets disabled, so make sure our count is reset.
      stepperPosition = 0;
    }
  }
}

void startMoveUp() {
  if (movementComplete == false) {                        // A down movement was interrupted.
    noInterrupts();                                       // Figure out where we're at, so we can stop the movement.
    int32_t currentPos = stepperPosition;                 // Record the current position and speed of the stepper.
    int32_t currentSpeed = stepperSpeed;
    interrupts();
    stepperSpeedStep = ACCEL_SPEED_STEP;                  // Normal acceleration.
    if (currentPos < ACCEL_STEPS) {                       // We were still accelerating.
      targetPosition = currentPos * 2;                    // Continue moving for as much as we did to slow down.
      startDecelerating = currentPos;                     // Start deceleration here and now.
    }
    else if (currentPos < startDecelerating) {            // We were in normal movement, slow down now.
      startDecelerating = currentPos;                     // Start deceleration here and now.
      targetPosition = currentPos + ACCEL_STEPS;          // Target position: the end of the deceleration.
    }
    displayBlinks = true;                                 // Blink the display as we're interrupted.
    while (movementComplete == false) {}                  // Wait until the stepper has stopped. This blocks for up to 1 second...
  }
  targetPosition = 0;                                     // Move stepper up to home position.
  startMovement();
}

void startMovement() {
  stepperSpeed = STEPPER_MINIMUM_SPEED;
  minimumSpeed = STEPPER_MINIMUM_SPEED;
  stepperTargetSpeed = STEPPER_MOVE_SPEED;                // Move at this speed (steps per second).
  stepperSpeedStep = ACCEL_SPEED_STEP;
  movementComplete = false;
  if (targetPosition > stepperPosition) {                 // We're about to start moving DOWN.
    stepperDirection = DOWN;
    if (targetPosition - stepperPosition < ACCEL_STEPS * 2) { // Not enough distance for full acceleration and deceleration.
      startDecelerating = (targetPosition - stepperPosition) / 2;
    }
    else {
      startDecelerating = targetPosition - ACCEL_STEPS;
    }
    bitSet(TIMSK1, OCIE1A);                               // Stepper step interrupt: counts up for going down.
  }
  else {                                                  // We're about to start moving UP.
    stepperDirection = UP;
    if (stepperPosition - targetPosition < ACCEL_STEPS * 2) { // Not enough distance for full acceleration and deceleration.
      startDecelerating = (stepperPosition - targetPosition) / 2;
    }
    else {
      startDecelerating = targetPosition + ACCEL_STEPS;
    }
    bitSet(TIMSK1, OCIE1B);                               // Stepper step interrupt: counts down for going up.
  }
  bitSet(TIMSK0, OCIE0A);                                 // Acceleration interrupt.
  blinkLED(RED);
  digitalWrite(DIR, stepperDirection);
}
