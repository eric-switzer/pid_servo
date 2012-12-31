CC          = gcc
LINK        = gcc
#CCFLAGS     = -O2 -Wall -std=gnu99 -Iinclude/ -g
CCFLAGS     = -O2 -Wall -I. -Iinclude/ -g
TARGET      = pid_servo
TARGETC     = pid_servo.c
#LDFLAGS     = -lpthread -lm -lact_util -Llib/ -l2600 -levent -lhiredis -lnidaqmxbase
#LDFLAGS     = -lpthread -lmccusb -lm -static -lconfig
LDFLAGS     = -lhiredis -levent -lpthread -lm
SERVO_FLAGS =

HEADERS     = circular_buffer.h pmd.h usb-1208FS.h usb-1608FS.h \
              read_temp.h servo.h simulated_temp.h control.h servo_struct.h \
              extern.h

OBJS        = circular_buffer.o simulated_temp.o read_temp.o servo.o \
              control.o

all: control_struct $(TARGET)

# note that control_struct may not exist at `make` runtime
control_struct:
	python make_c_structs.py
	$(CC) $(CCFLAGS) $(SERVO_FLAGS) -c control_struct.c

$(TARGET): $(TARGETC) $(OBJS) $(HEADERS) Makefile $(LIB)
	$(LINK) $(CCFLAGS) $(SERVO_FLAGS) -o $(TARGET) $(TARGETC) $(OBJS) control_struct.o $(LDFLAGS)

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CCFLAGS) $(SERVO_FLAGS) -c $<

again: clean all

.PHONY: clean
clean:
	-rm -f *~ $(OBJS) $(TARGET) *.pyc control_struct.*
