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

double do_servo_stub() {
  int i;
  double residual, output;
  double val_now = 0.;

  for (i = 0; i < num_servo; i++) {
    printf("here %d %d\n", servo[i]->pid.active, servo[i]->pid.dead);
    do_servo_filter(i);
    residual = 0.;
    circ_buf_push(&servo[i]->pid.resid, residual);

    if (i==0) {
      val_now = servo[i]->filt.val;
    }

    // if the temperature readout is malfunctioning (-1)
    if(circ_buf_get(&servo[i]->filt.buf, double, 1)==-1) {
      servo[i]->pid.dead = INACTIVE;
      //message(M_WARN, "Servoing off of a dead channel");
      output = DRV_LOWER;
    } else {
      // if a channel becomes live again, switch it back
      servo[i]->pid.dead = ACTIVE;
    }
    servo[i]->output = output;

    if((servo[i]->pid.active == ACTIVE) && (servo[i]->pid.dead != INACTIVE))
      printf("setting value");

  }
  return val_now;
}

        /* max_atan = 2 * param_val_curr[servo[i]->pid.sat_idx] / M_PI;
        residual = max_atan * atan((param_val_curr[servo[i]->pid.set_idx] -
                                    servo[i]->filt.val) / max_atan); */


