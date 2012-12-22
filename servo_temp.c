#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "simulated_temp.h"
#include "read_temp.h"
#include "servo_temp.h"

void servo_temp_init () {
  printf("initializing servo thread\n");
}

void *servo_temp_thread(void *arg) {
  double current_value;

  while (1) {
    usleep(10000);
    current_value = circ_buf_get(&temperature_buf, double, 1);
    printf("servo: %10.5f %10.5f\n", current_temperature, current_value);
  }
}
