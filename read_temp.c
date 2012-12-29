#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

// ADD THIS FOR THE SERVO
#include "servo.h"
#include "servo_struct.h"

void read_temp_init () {
  printf("initializing read thread\n");
}

void *read_temp_thread(void *arg) {
  FILE *outfile;
  outfile=fopen(READTHREAD_OUTPUT, "w");
  struct timeval tv_now, tv_start;
  double time_now;

  gettimeofday(&tv_start,NULL);

  while (1) {
    gettimeofday(&tv_now,NULL);
    time_now = (double)(tv_now.tv_sec - tv_start.tv_sec);
    time_now += ((double)(tv_now.tv_usec - tv_start.tv_usec))/1.e6;

    // ADD THIS FOR THE SERVO
    PUSH_SERVO_TEMP(srv_detector1_idx, current_reading);

    fprintf(outfile, "%10.15f %10.5f\n",
            time_now, current_reading);

    fflush(outfile);
    usleep(10000);
  }
  fclose(outfile);
}
