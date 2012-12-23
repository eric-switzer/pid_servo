#ifndef SERVO_TEMP_H
#define SERVO_TEMP_H

#define SERVOTHREAD_OUTPUT "servothread.dat"
void servo_temp_init();
void *servo_temp_thread(void *arg);

#endif
