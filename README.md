# ati0216_stepper_control

## Connecting the board
There are a total of four green connectors on the board. 

1. Mode: connects to a switch that sets either PASTA mode (switch open) or EGGS mode (switch closed). In PASTA mode the basket will move up and down regularly during cooking; in EGGS mode it does not move.

1. Home: connects to a a NO limit switch to indicate the home position, the topmost position the stepper should reach. The switch should close when the position is reached.

1. 3-pin power connection: +12V power (must be able to supply at least 1.5A), +5V (power draw estimate <200 mA), and a shared ground connection.

1. 4-pin stepper connection: A1 and A2 for coil A, B1 and B2 for coil B.

There are also two solder jumpers on the board.
1. 16x microstepping: if closed the board will use 16x microstepping; if open (default) this is 32x.
1. Limit switch: this board has additional circuitry to disable the stepper when the limit switch is hit, so the controller can not make the stepper continue to move up when the limit switch is reached.

### Limit switch
* Both 1 and 2 open: function is disabled.
* To use this function, one of the two sides should be closed, depending on the DIR signal.
  * If DIR == HIGH drives the stepper into the limit switch: close side 2.
  * If DIR == LOW drives the stepper into the limit switch: close side 1.


## Setting up the software
There is a number of settings in stepper_control.ino that may be changed to set the specific behaviour of various functions such as speed and timing behaviour. Set them with care.

## Uploading the software
Set up the IDE:
* Tools > Board: "ATtiny24/44/84"
* Tools > Chip: "ATtiny84"
* Tools > Clock: "8 MHz (internal)"

Connect the USBASP to an available USB port of the computer; use the 6p-10p adapter to connect it to the green board with 2x3 pogopins. Press the pogopins gently into the openings next to the encoder; the pin marked MISO goes to pin 1 on the board (the one marked with a square around it). Then in the IDE click the Upload button to upload the sketch.

It should be possible to program with the Mode switch connected and closed (untested); it shares one of the MCU connections with the programming header. If the stepper is connected and powered up it will move noisily during programming as also here some pins are shared.

## Board behaviour

### Starting up
Upon startup, it first searches for its home position (all the way up). The board does not react to any user input until the home has been found.

* When the board is powered up, the stepper is moving up to search for it's home position. The display will show 88:88 at this time, and the LED is blinking red/green.
* The moment the limit switch is closed, the stepper will move down again until the switch opens again. This is marked as home position. The display will show a time in minutes:seconds, this is the currently set up cooking time.

The board is now ready to use.

### Normal use
The user sets up the time for cooking, presses the knob, the basket moves down for cooking, and when time is up it moves back up again.

* Turning the knob changes this time; turn left to reduce the time, right to increase the time. The LED is on, green.
* When the desired time is set, press the knob to start the cooking process. At this moment the PASTA/EGG mode is set according to the switch, there is no indication on the board related to this. Changing this switch after the cooking has started has no effect.
* The stepper is moving the basket down, as the LED blinks red. Pressing the knob interrupts the cooking, the basket is moved back up immediately.
* When the stepper is all the way down, the cooking time starts. The LED is on red, and the display counts down the minutes and seconds to zero. Pressing the knob for 3 seconds interrupts the cooking, moving the basket up immediately.
* When time is up (or the user interrupted the process), the basket moves up as the LED blinks red. The display now shows 00:00.
* When the basket is all the way up, the LED blinks green, and the display blinks showing the original cooking time. The user can now start a new cooking process by either rotating or pressing the knob. In both cases the display and the LED stop blinking; the user can now set a new time or press the knob again to start new cooking process right away.

## Other notes
The handleError function is at this moment not used, this is for possible future use. The stepper controller chip has many advanced functions, including stall detection, that are in this version of the board not used. 

One error it could detect now is not reaching the home switch within the expected rotation distance: there's a limit to how far the basket can move up. This would imply a failure of the home switch, or a failure of the stepper itself.
