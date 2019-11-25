/*
    ENTER_TIME
      LED continuous green.
      Motor stopped.
      Display shows current set time.
      Set time with encoder: encoder one click per second increase or decrease.
      If encoder pressed: MOVE_UP.
*/
void handleEnterTime() {
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
}
