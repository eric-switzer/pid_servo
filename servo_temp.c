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
#include "servo_temp.h"

void servo_temp_init () {
  printf("servo thread initializing\n");
  // TODO: replace with buffer of structs? sizeof struct?
  circ_buf_init(temperature_buf, sizeof(double), TEMPERATURE_BUFLEN);

void init_servo(struct servo_t *ptr, const char *source, double *coef,@
                int set_idx, int p_idx, int i_idx, int d_idx, int sat_idx,@
                int mem_idx) {
  int i, len;
@
  servo = (struct servo_t **)realloc(servo, (++num_servo) *
                                            sizeof(struct servo_t *));
  i = num_servo - 1;
  servo[i] = ptr;
  strncpy(servo[i]->temp.source, source, FIELD_LEN - 1);
@@
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

}

void *servo_temp_thread(void *arg) {
  double residual;

  while (1) {
    usleep(100000);

    residual = circ_buf_get(temperature_buf, double, 1);
    printf("servo thread %10.15f\n", residual);
  }
}
