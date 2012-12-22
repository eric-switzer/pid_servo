#ifndef READ_TEMP_H
#define READ_TEMP_H

#include "circular_buffer.h"
struct circ_buf_t temperature_buf;
#define TEMPERATURE_BUFLEN 1000

void read_temp_init();
void *read_temp_thread(void *arg);

#endif
