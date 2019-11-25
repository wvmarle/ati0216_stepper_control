void handleError() {
  uint8_t displayData[] = {14, 16, 16, errorCode};          // Set initial display to Err0
  clockPoint = POINT_OFF;
  updateDisplay();
  setLED(RED);
  while (true) {}                                           // Halt operations.
}
