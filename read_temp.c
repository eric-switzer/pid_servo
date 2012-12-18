#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include "read_temp.h"
#include "circular_buffer.h"

// open the device and build the calibration table
void read_temp_init () {
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
}

void *read_temp_thread(void *arg) {
  float temp_value = 0., residual;
  signed short svalue;
  struct timeval tv;

  while (1) {
    gettimeofday(&tv,NULL);
    //usleep(20);
    svalue = usbAIn_USB1608FS(fd[0], (__u8) TEMP_CHANNEL, BP_10_00V, table_AIN);
    temp_value = volts_USB1608FS(BP_10_00V, svalue);
    circ_buf_push(temperature_buf, temp_value);

    residual = circ_buf_get(temperature_buf, double, 1);
    //printf("%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, temp_value);
    printf("%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, residual);
  }
}
