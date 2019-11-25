/*
    HOMING
      LED blinks red
      Motor finds home position.
      Switch LED to green - machine ready to use.
*/
void handleHoming() {
  OCR1A = CLOCKSPEED / (HOMING_SPEED * 2);
  if (movementComplete) {                                   // The step is completed - check whether we're home now.
    if (homing) {                                           // Moving down - home switch not found yet.
      if (digitalRead(HOME) == HIGH) {                      // Switch triggered: we're home. Start moving up steps until released.
        homing = false;                                     // Home found.
        stepperDirection = DOWN;                            // Now we have to start moving down until the switch is not triggered any more.
        digitalWrite(DIR, DOWN);
      }
      else {                                                // We're not home yet.
        targetPosition = stepperPosition--;                 // Move up (one step at least, move until the next TIMER0 interrupt).
        bitSet(TIMSK1, OCIE1B);                             // Enable the stepper step interrupt for moving up.
        movementComplete = false;
      }
    }
    else {                                                  // We're moving down now until the switch is not triggered any more.
      if (digitalRead(HOME) == HIGH) {                      // Still too high.
        targetPosition = stepperPosition++;                 // Move down (one step at least, move until the next TIMER0 interrupt).
        bitSet(TIMSK1, OCIE1A);                             // Enable the stepper step interrupt for moving down.
        movementComplete = false;
        setLED(RED);
      }
      else {                                                // We're in position.
        stepperPosition = 0;                                // Set current position as zero (home).
        targetPosition = 0;                                 // Make sure the target matches.
        processState = ENTER_TIME;                          // Ready to go!
        clockPoint = POINT_ON;
        displayTime(cookingTime);
        setLED(GREEN);
      }
    }
  }
}
