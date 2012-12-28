CC          = gcc -c
LINK        = gcc
#CCFLAGS     = -O2 -Wall -std=gnu99 -Iinclude/ -g
CCFLAGS     = -O2 -Wall -I. -Iinclude/ -g
TARGET      = pid_servo
TARGETC     = pid_servo.c
#LDFLAGS     = -lpthread -lm -lact_util -Llib/ -l2600 -levent -lhiredis -lnidaqmxbase
#LDFLAGS     = -lpthread -lmccusb -lm -static -lconfig
LDFLAGS     = -lhiredis -levent -lpthread -lm
SERVO_FLAGS =

HEADERS     = uthash.h circular_buffer.h pmd.h usb-1208FS.h usb-1608FS.h \
              read_temp.h servo.h simulated_temp.h control.h

OBJS        = circular_buffer.o simulated_temp.o read_temp.o servo.o \
              control.o

all: $(TARGET)

$(TARGET): $(TARGETC) $(OBJS) $(HEADERS) Makefile $(LIB)
	$(LINK) $(CCFLAGS) $(SERVO_FLAGS) -o $(TARGET) $(TARGETC) $(OBJS) $(LDFLAGS)

%.o: %.c $(HEADERS) Makefile
	$(CC) $(CCFLAGS) $(SERVO_FLAGS) -c $<

again: clean all

.PHONY: clean
clean:
	-rm -f *~ $(OBJS) $(TARGET)
