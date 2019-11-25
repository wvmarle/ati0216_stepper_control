/*
  MOVE_DOWN
    LED blinks red.
    Motor moves some preset distance.
    Display shows current set time.
    If encoder pressed: INTERRUPTED.
    If encoder is turned: ignored.
    When distance reached: COUNTDOWN.
*/

void handleMoveDown() {
  if (movementComplete) {                                   // We're there!
    processState = COUNTDOWN;
    countdownStartTime = myMillis();
    lastShake = myMillis();
    setLED(RED);
  }
  if (switchState == LOW &&                                 // Switch was just pressed.
      oldSwitchState == HIGH) {
    oldSwitchState = LOW;
    startMoveUp();                                          // Start moving up - cancelling the cooking process.
    processState = MOVE_UP;
  }
}

void startMoveDown() {
  targetPosition = STEPPER_MOVE_STEPS;                      // Move stepper to the DOWN position.
  startMovement();
}
