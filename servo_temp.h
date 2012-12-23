#ifndef SERVO_TEMP_H
#define SERVO_TEMP_H

#include "circular_buffer.h"
struct circ_buf_t temperature_buf;
#define TEMPERATURE_BUFLEN 1000

void servo_temp_init();
void *servo_temp_thread(void *arg);

#endif
