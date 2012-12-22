pid_servo
=========
A simple code for PID servo loops. (in development)

redis passes commands to change the PID parameters or the setpoint.

Hardware setup:
===============
usb-1608FS reads the temperature using an AD590 read across a 9.8 kOhm resistor.
usb-1208FS applies its 0-4.1 V range to a 590 Ohm resistor glued to the AD590 giving 28 mW. The time constant is roughly one minute.

