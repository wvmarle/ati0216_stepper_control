/*
  COMPLETED
    LED blinks green.
    Motor stopped.
    Display shows original set time.
    If encoder pressed or turned: ENTER_TIME.
*/
void handleCompleted() {
  blinkLED(GREEN);
  displayBlinks = true;

#ifdef DEMO_MODE
  static uint32_t demoCompletedTime;
  static bool demoCompletedDelay = false;
  static uint32_t demoCompletedDelayTime;
  if (demoCompletedDelay == false) {
    demoCompletedDelay = true;
    demoCompletedTime = myMillis();
    demoCompletedDelayTime = random(5000, 10000);           // Random 5-10 second time spent UP with display blinking.
  }
  else {
    if (myMillis() - demoCompletedTime > demoCompletedDelayTime) {  // Time to move on.
      demoCompletedDelay = false;                           // Reset for the next round.
      processState = ENTER_TIME;                            // Start entering the time.
      setLED(GREEN);
      displayBlinks = false;
      displayTime(cookingTime);
    }
  }
#else
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
#endif
}
