# ati0216_stepper_control

Connecting the board:
There are a total of four green connectors on the board. Two 2-pin, one 3-pin and one 4-pin connector, each marked with their respective function.

Mode: a switch that sets either PASTA mode (switch open) or EGGS mode (switch closed). In PASTA mode the basket will move up and down regularly during cooking; in EGGS mode it does not move.

Home: connetion for a NO limit switch to indicate the home position, the topmost position the stepper should reach. The switch should close when the position is reached.

3-pin power connection: +12V power (must be able to supply at least 1.5A), +5V (power draw estimate <200 mA), and a shared ground connection.

4-pin stepper connection: A1 and A2 for coil A, B1 and B2 for coil B.

There are two solder jumpers on the board.
- 16x microstepping: if closed the board will use 16x microstepping; if open (default) this is 32x.
- Limit switch: this board has additional circuitry to disable the stepper when the limit switch is hit, so the controller can not make the stepper continue to move up when the limit switch is reached.
Both 1 and 2 open: function is disabled.
To use this function, one of the two sides should be closed, depending on the DIR signal.
If DIR == HIGH drives the stepper into the limit switch: close side 2.
If DIR == LOW drives the stepper into the limit switch: close side 1.


Setting up the software:
There is a number of settings in stepper_control.ino that may be changed to set the specific behaviour of various functions such as speed and timing behaviour. Set them with care.


Board behaviour:
When the board is powered up, the stepper is moving up to search for it's home position. The display will show 88:88 at this time, and the LED is blinking red/green.
The moment the limit switch is closed, the stepper will move down again until the switch opens again. This is marked as home position. The display will show a time in minutes:seconds, this is the currently set up cooking time.
Turning the knob changes this time; turn left to reduce the time, right to increase the time. The LED is on, green.
When the desired time is set, press the knob to start the cooking process. At this moment the PASTA/EGG mode is set according to the switch, there is no indication on the board related to this. Changing this switch after the cooking has started has no effect.
The stepper is moving the basket down, as the LED blinks red. Pressing the knob interrupts the cooking, the basket is moved back up immediately.
When the stepper is all the way down, the cooking time starts. The LED is on red, and the display counts down the minutes and seconds to zero. Pressing the knob for 3 seconds interrupts the cooking, moving the basket up immediately.
When time is up (or the user interrupted the process), the basket moves up as the LED blinks red. The display now shows 00:00.
When the basket is all the way up, the LED blinks green, and the display blinks showing the original cooking time.
The user can now start a new cooking process by either rotating or pressing the knob. In both cases the display and the LED stop blinking; the user can now set a new time or press the knob again to start new cooking process right away.


Note: the handleError function is at this moment not used, this is for possible future use. The stepper controller has many advanced functions, including stall detection, that are in this version of the board not used. 
One error it could detect now is not reaching the home switch within the expected rotation distance: there's a limit to how far the basket can move up. This would imply a failure of the home switch, or a failure of the stepper itself.
