#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "simulated_temp.h"
#include "read_temp.h"
#include "circular_buffer.h"

// open the device and build the calibration table
void read_temp_init () {
  printf("initializing read thread\n");
  circ_buf_init(&temperature_buf, sizeof(double), TEMPERATURE_BUFLEN);
}

void *read_temp_thread(void *arg) {
  while (1) {
    usleep(10000);
    circ_buf_push(&temperature_buf, current_reading);
    printf("read: %10.5f\n", current_reading);
  }
}
