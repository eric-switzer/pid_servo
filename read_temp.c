#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

#include "servo.h"
#include "servo_struct.h"

// open the device and build the calibration table
void read_temp_init () {
  printf("initializing read thread\n");
}

void *read_temp_thread(void *arg) {
  FILE *outfile;
  outfile=fopen(READTHREAD_OUTPUT, "w");
  struct timeval tv;
  double alt_temp;

  while (1) {
    gettimeofday(&tv,NULL);
    PUSH_SERVO_TEMP(srv_detector1_idx, current_reading);

    alt_temp = current_reading * 2.;
    PUSH_SERVO_TEMP(srv_detector2_idx, alt_temp);

    fprintf(outfile, "%ld.%ld %10.5f %10.5f\n",
            tv.tv_sec, tv.tv_usec, current_reading, alt_temp);

    fflush(outfile);
    usleep(10000);
  }
  fclose(outfile);
}
