`pid_servo`
===========
This is a C implementation of a PID control loop. The main code, `pid_servo` starts a read thread, a servo thread and simulation thread. The read thread can be replaced by code to read from data acquisition hardware. `read_hardware.c` is and example for reading a usb-1608FS ADC. The servo thread can have several systems controlled simultaneously, with parameters specified in the `servo_struct`. Commanding of the servo thread is done through `redis` and `control.c`. The PID parameters that can be commanded through this interface are in `pid_servo.json`, which is parsed into a `constrol_struct` through `master_config_parse.py`. The GUI `https://github.com/eric-switzer/viscacha` is convenient for issuing commands to the PID loop. The simulation thread here models a simple thermal system at fixed bath temperature. This can be replaced by a real thermal system with the mock-up described below.

All of the threads are asynchronous. The read thread pushes values to a circular buffer. When the servo needs the current sensor value, it applies an FIR filter over the data in this buffer, and pulls off that value. The servo loop also keeps a circular buffer for its integral term that allows a finite (FIR) memory. The commanding thread acquires a lock to write to the servo parameter array. Depending on the implementation, other variables may need to be locked. The commanding thread can dispatch to other system threads (in addition to the servo).

Important note: set the timeout in `redis.conf` to 0. The commanding thread needs a persistent connection.

Mock temperature control hardware setup:
=======================================
usb-1608FS reads the temperature using an AD590 read across a 9.8 kOhm resistor.
usb-1208FS applies its 0-4.1 V range to a 590 Ohm resistor glued to the AD590 giving 28 mW. The time constant is roughly one minute.

