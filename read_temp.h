#ifndef READ_TEMP_H
#define READ_TEMP_H

#include <sys/types.h>
#include <asm/types.h>

#include "pmd.h"
#include "usb-1608FS.h"
#include "circular_buffer.h"

#define TEMP_CHANNEL 0
#define MAX_DEVICES 7

// calibration and file for the USB ADC device
Calibration_AIN table_AIN[NGAINS_1608FS][NCHAN_1608FS];
int fd[MAX_DEVICES], fp;

// buffer within which to push temperature values
struct circ_buf_t *temperature_buf;
#define TEMPERATURE_BUFLEN 1000

void read_temp_init();
void *read_temp_thread(void *arg);

#endif
