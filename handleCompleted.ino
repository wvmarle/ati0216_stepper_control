/*
  COMPLETED
    LED blinks green.
    Motor stopped.
    Display shows original set time.
    If encoder pressed: MOVE_UP.
    If encoder turned: ENTER_TIME.
*/
void handleCompleted() {
  blinkLED(GREEN);
  displayBlinks = true;

  ////////////////////////////
  // if encoder pressed: start next round; if moved set new time.
  //  if (switchState == LOW &&                                 // Switch was just pressed.
  //      oldSwitchState == HIGH) {
  //    oldSwitchState = LOW;
  //    startMoveDown();
  //    processState = MOVE_DOWN;
  //  }
  //  if (oldEncoderReading != encoderReading) {                // Encoder moved.
  //    cookingTime += (encoderReading - oldEncoderReading) * TIME_STEP; // Adjust the time accordingly.
  //    processState = ENTER_TIME;                              // Start entering the time.
  //    setLED(GREEN);
  //  }                                                         // Do not record the new reading as old reading, as we still need the new reading in the next round!

  ////////////////////////////
  // if encoder pressed or moved: set new time.
  if ((switchState == LOW && oldSwitchState == HIGH) ||     // Switch was just pressed, or
      (oldEncoderReading != encoderReading)) {              // encoder moved:
    oldSwitchState = switchState;
    processState = ENTER_TIME;                              // Start entering the time.
    handleEnterTime();                                      // Handle the encoder reading, if applicable.
    setLED(GREEN);
    displayBlinks = false;
    displayTime(cookingTime);
  }                                                         // Do not record the new reading as old reading, as we still need the new reading in the next round!
}
