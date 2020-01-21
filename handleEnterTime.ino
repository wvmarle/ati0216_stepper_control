/*
    ENTER_TIME
      LED continuous green.
      Motor stopped.
      Display shows current set time.
      Set time with encoder: encoder one click per second increase or decrease.
      If encoder pressed: MOVE_UP.
*/
void handleEnterTime() {
#ifdef DEMO_MODE
  static uint32_t demoEnterTimeStart;
  static bool demoEnteringTime = false;
  static uint32_t demoEnterTimeDelay;
  static uint32_t demoLastCookingTimeChange;
  static uint8_t newCookingTime;
  if (demoEnteringTime == false) {
    demoEnteringTime = true;
    demoEnterTimeStart = myMillis();
    demoEnterTimeDelay = random(5000, 15000);               // Random 5-15 second time spent entering time.
    newCookingTime = cookingTime;
    while (cookingTime == newCookingTime) {                 // Make sure we have a diferent time.
      newCookingTime = random(MIN_TIME / TIME_STEP, MAX_TIME / TIME_STEP) * TIME_STEP;
    }
  }
  else {
    if (myMillis() - demoEnterTimeStart > 4000) {           // First 4 seconds: do nothing.
      if (myMillis() - demoLastCookingTimeChange > 1000) {  // Change every 1 second.
        demoLastCookingTimeChange = myMillis();
        if (cookingTime > newCookingTime) {
          cookingTime -= TIME_STEP;
        }
        else if (cookingTime < newCookingTime) {
          cookingTime += TIME_STEP;
        }
        clockPoint = POINT_ON;
        displayTime(cookingTime);
      }
    }
    if (myMillis() - demoEnterTimeStart > demoEnterTimeDelay) {  // Time to move on.
      demoEnteringTime = false;                             // Reset for the next round.
      startMoveDown();                                      // Start moving down.
      processState = MOVE_DOWN;
    }
  }
#else
  if (encoderReading != oldEncoderReading) {                // Encoder moved (old reading is recorded at the end of loop()).
    cookingTime += (oldEncoderReading - encoderReading) * TIME_STEP; // Adjust the time accordingly.
    cookingTime = constrain(cookingTime, MIN_TIME, MAX_TIME); // Make sure we're within limits.
    clockPoint = POINT_ON;
    displayTime(cookingTime);
  }
  if (switchState == LOW &&                                 // Switch was just pressed.
      oldSwitchState == HIGH) {
    oldSwitchState = LOW;
    EEPROM.put(EEPROM_COOKINGTIME, cookingTime);            // Store the time we set in EEPROM.
    startMoveDown();                                        // Start moving down.
    processState = MOVE_DOWN;
  }
#endif
}
