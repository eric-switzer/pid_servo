`pid_servo`
===========
A simple code for PID servo loops. (in development)

get: libyaml-dev?

Components:
===========
* read thread: Reads the sensors and pushes control loop sensor values to a dictionary of circular buffers.
* servo thread: The servo thread is asynchronous, and applies an FIR over the history in the circular buffer each time it needs to sample the current sensor value.
* command thread: This accepts and acks commands through two redis connections. The command is received on a blocking subscriber connection and the ack thread publishes and sets the redis server value once that parameter has been set.
* simulator thread: with no real hardware to interact with, this can be configured to test the servo behavior first.

The overall design is meant to drop in easily to existing readout software

Hardware setup:
===============
usb-1608FS reads the temperature using an AD590 read across a 9.8 kOhm resistor.
usb-1208FS applies its 0-4.1 V range to a 590 Ohm resistor glued to the AD590 giving 28 mW. The time constant is roughly one minute.

