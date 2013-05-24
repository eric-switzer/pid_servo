#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

#ifndef __NO_HARDWARE__
#include <sys/types.h>
#include <asm/types.h>
#include "pmd.h"
#include "usb-1608FS.h"
#define TEMP_CHANNEL 0
#define MAX_DEVICES 7

Calibration_AIN table_AIN[NGAINS_1608FS][NCHAN_1608FS];
int fd[MAX_DEVICES], fp;
#endif // NO_HARDWARE

// ADD THIS FOR THE SERVO
#include "servo.h"
#include "servo_struct.h"

void read_temp_init () {
  printf("initializing read thread\n");

#ifndef __NO_HARDWARE__
  int i, j;
  int nInterfaces = 0;

  nInterfaces = PMD_Find(MCC_VID, USB1608FS_PID, fd);
  if (nInterfaces <= 0) {
    fprintf(stderr, "USB 1608FS not found. (did you use sudo?)\n");
    exit(1);
  } else {
    printf("USB 1608FS Device is found! Number of Interfaces = %d\n", nInterfaces);
  }

  usbDConfigPort_USB1608FS(fd[0], DIO_DIR_OUT);
  usbDOut_USB1608FS(fd[0], 0);

  usbBuildCalTable_USB1608FS(fd[0], table_AIN);
  for (i = 0; i < 4; i++) {
    for (j = 0; j < NCHAN_1608FS; j++) {
      printf("cal[%d][%d].slope = %f    table_AIN[%d][%d].offset = %f\n",
             i, j, table_AIN[i][j].slope, i, j, table_AIN[i][j].offset);
    }
  }
#endif // NO_HARDWARE

}

void *read_temp_thread(void *arg) {
  FILE *outfile;
  outfile=fopen(READTHREAD_OUTPUT, "w");
  struct timeval tv_now, tv_start;
  double time_now;
  double temp_value = 0.;
#ifndef __NO_HARDWARE__
  signed short svalue;
#endif

  gettimeofday(&tv_start,NULL);

  while (1) {
    gettimeofday(&tv_now,NULL);
    time_now = (double)(tv_now.tv_sec - tv_start.tv_sec);
    time_now += ((double)(tv_now.tv_usec - tv_start.tv_usec))/1.e6;

#ifndef __NO_HARDWARE__
    svalue = usbAIn_USB1608FS(fd[0], (__u8) TEMP_CHANNEL, BP_10_00V, table_AIN);
    temp_value = (double)volts_USB1608FS(BP_10_00V, svalue);
#else
    usleep(10000);
    temp_value = current_reading;
#endif

    // ADD THIS FOR THE SERVO
    PUSH_SERVO_TEMP(srv_detector1_idx, temp_value);

    fprintf(outfile, "%10.15f %10.5f\n",
            time_now, temp_value);

    fflush(outfile);
  }
  fclose(outfile);
}
