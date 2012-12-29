//#include <act_util.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "control_struct.h"
#include "servo.h"

#define SERVO_DECLARATION
#include "servo_struct.h"
#undef SERVO_DECLARATION

void init_servo(struct servo_t *ptr, double *coef,
                int set_idx, int p_idx, int i_idx, int d_idx, int sat_idx,
                int mem_idx) {
  int i, fir_len;

  servo = (struct servo_t **)realloc(servo, (++num_servo) *
                                            sizeof(struct servo_t *));
  i = num_servo - 1;
  servo[i] = ptr;

  for (fir_len = 0; coef[fir_len]; fir_len++);
  servo[i]->filt.coef = coef;

  circ_buf_init(&servo[i]->filt.buf, sizeof(double), fir_len);
  servo[i]->filt.val = 0.;

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

void servo_init() {
  servo = NULL;

  #define SERVO_STRUCT_LIST
  #include "servo_struct.h"
  #undef SERVO_STRUCT_LIST
}

void do_servo_filter(int servo_index) {
  int i, j;
  double filt_val;
  struct circ_buf_t *cb;

  i = servo_index;
  cb = &servo[i]->filt.buf;

  // FIR fiter of the buffer
  // 0 is the oldest and 1 is the newest
  filt_val = circ_buf_get(cb, double, 0) * servo[i]->filt.coef[circ_buf_len(cb) - 1];
  for (j = 0; j < (circ_buf_len(cb) - 1); j++)
    filt_val += circ_buf_get(cb, double, j + 1) * servo[i]->filt.coef[j];

  servo[i]->filt.val = filt_val;
  //message(M_INFO, "%d %10.15f %10.15f", i, val, filt_val);
}

void do_driver_shutdown(double *param_val) {
  int i;

  for (i=0; i < num_servo; i++) {
    servo[i]->pid.active = INACTIVE;
  }

  return;
}

void do_servo(double *param_val) {
#ifndef __NO_SERVO__
  int i, j;
  double residual, integral, derivative, output, max_atan;

  for (i = 0; i < num_servo; i++) {
    max_atan = 2 * param_val[servo[i]->pid.sat_idx] / M_PI;
    residual = max_atan * atan((param_val[servo[i]->pid.set_idx] -
                               servo[i]->filt.val) / max_atan);
    circ_buf_push(&servo[i]->pid.resid, residual);
    for (j = 0, integral = 0; j < SERVO_RESIDUAL_DEPTH; j++) {
      integral += circ_buf_get(&servo[i]->pid.resid, double, j) *
                  pow(param_val[servo[i]->pid.mem_idx], (double)j);
    }

    derivative = residual - circ_buf_get(&servo[i]->pid.resid, double, 1);

    output = param_val[servo[i]->pid.p_idx] * residual +
             param_val[servo[i]->pid.i_idx] * integral +
             param_val[servo[i]->pid.d_idx] * derivative;

    output = output < DRV_LOWER ? DRV_LOWER : output;
    output = output > DRV_UPPER ? DRV_UPPER : output;

    // if the temperature readout is malfunctioning (-1)
    if(circ_buf_get(&servo[i]->filt.buf, double, 0)==-1) {
      servo[i]->pid.dead = INACTIVE;
      //message(M_WARN, "Servoing off of a dead channel");
      output = DRV_LOWER;
    } else {
      // if a channel becomes live again, switch it back
      servo[i]->pid.dead = ACTIVE;
    }
    servo[i]->output = output;
    //message(M_INFO, "** %d %10.15f %10.15f %10.15f\n", i,
    //                param_val[servo[i]->pid.set_idx],
    //                servo[i]->filt.val,
    //                servo[i]->output);

    // implement this
    //do_driver_shutdown(param_val);

    if((servo[i]->pid.active == ACTIVE) && (servo[i]->pid.dead != INACTIVE))
      printf("setting value");

  }
#endif
}

double do_servo_stub() {
  int i;
  double residual, output;
  double val_now = 0.;

  for (i = 0; i < num_servo; i++) {
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

void *servo_thread(void *arg) {
  double det1_now, det2_now, previous_value, pprev, val_now;
  double time_now;

  FILE *outfile;
  outfile=fopen(SERVOTHREAD_OUTPUT, "w");
  struct timeval tv_now, tv_start;

  gettimeofday(&tv_start,NULL);

  while (1) {
    gettimeofday(&tv_now,NULL);
    time_now = (double)(tv_now.tv_sec - tv_start.tv_sec);
    time_now += ((double)(tv_now.tv_usec - tv_start.tv_usec))/1.e6;

    det1_now = GET_SERVO_TEMP(srv_detector1_idx, 0);
    previous_value = GET_SERVO_TEMP(srv_detector1_idx, 1);
    pprev = GET_SERVO_TEMP(srv_detector1_idx, 2);
    det2_now = GET_SERVO_TEMP(srv_detector2_idx, 0);

    val_now = do_servo_stub();

    fprintf(outfile, "%10.15f %10.5f %10.5f %10.5f %10.5f\n",
                     time_now, det1_now, previous_value, pprev, val_now);

    fflush(outfile);
    usleep(100000);
  }

  fclose(outfile);
}

