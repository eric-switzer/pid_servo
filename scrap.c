// Headers needed for USB ADC/DAC
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

// Header 
#include <sys/types.h>
#include <asm/types.h>

#include "pmd.h"
#include "usb-1608FS.h"


#define TEMP_CHANNEL 0
#define MAX_DEVICES 7

// calibration and file for the USB ADC device
Calibration_AIN table_AIN[NGAINS_1608FS][NCHAN_1608FS];
int fd[MAX_DEVICES], fp;

// buffer within which to push temperature values

  
// INIT:
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

  // READ
  float temp_value = 0., residual;
  signed short svalue;
  struct timeval tv;
    gettimeofday(&tv,NULL);
    //usleep(20);
    svalue = usbAIn_USB1608FS(fd[0], (__u8) TEMP_CHANNEL, BP_10_00V, table_AIN);
    temp_value = volts_USB1608FS(BP_10_00V, svalue);
    circ_buf_push(temperature_buf, temp_value);

    residual = circ_buf_get(temperature_buf, double, 1);
    //printf("%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, temp_value);
    printf("%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, residual);


struct filter_t {
  double *coef;
  struct circ_buf_t buf;
  double val;
};

struct pid_servo_t {
  struct circ_buf_t resid;
  double setpoint;
  double p_term;
  double i_term;
  double d_term;
  int sat_idx;
  int mem_idx;
  int active;
  int dead;
};

struct servo_t {
  struct filter_t filt;
  struct pid_servo_t pid;
  double output;
};

int num_servo;
struct servo_t **servo;

void init_servo(struct servo_t *ptr, const char *source, double *coef,@
                int set_idx, int p_idx, int i_idx, int d_idx, int sat_idx,@
                int mem_idx) {
  int i, len;
  servo = (struct servo_t **)realloc(servo, (++num_servo) *
                                            sizeof(struct servo_t *));
  i = num_servo - 1;
  servo[i] = ptr;
  strncpy(servo[i]->temp.source, source, FIELD_LEN - 1);

  // Count the length of the filter.
  for (len = 0; coef[len]; len++);
  servo[i]->filt.coef = coef;

  // Initialise the filter.
  circ_buf_init(&servo[i]->filt.buf, sizeof(double), len);
  servo[i]->filt.val = 0;

  // Initialise PID stuff.
  circ_buf_init(&servo[i]->pid.resid, sizeof(double), SERVO_RESIDUAL_DEPTH);
  servo[i]->pid.set_idx = set_idx;
  servo[i]->pid.p_idx = p_idx;
  servo[i]->pid.i_idx = i_idx;
  servo[i]->pid.d_idx = d_idx;
  servo[i]->pid.sat_idx = sat_idx;
  servo[i]->pid.mem_idx = mem_idx;
  // assume by default that the servo is off, the the channel is not dead
  servo[i]->pid.active = INACTIVE;
  servo[i]->pid.dead   = ACTIVE;
}

