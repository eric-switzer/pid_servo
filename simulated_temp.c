#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "simulated_temp.h"
#include <sys/time.h>

// open the device and build the calibration table
void simulate_temp_init () {
  printf("initializing sim thread\n");
}

void *simulate_temp_thread(void *arg) {
  double time_now;
  struct timeval tv_start, tv_now;
  gettimeofday(&tv_start,NULL);

  while (1) {
    usleep(10000);
    gettimeofday(&tv_now,NULL);
    time_now = (double)(tv_now.tv_sec - tv_start.tv_sec);
    time_now += ((double)(tv_now.tv_usec - tv_start.tv_usec))/1.e6;
    current_temperature = sin(time_now/2./M_PI);

    current_reading = current_temperature;

    printf("t=%10.15f P=%10.5f T=%10.5f Tm=%10.5f\n",
           time_now, current_power, current_temperature, current_reading);
  }
}
