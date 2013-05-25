//#include <act_util.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "control_struct.h"
#include "control.h"
#include "servo.h"
#include "simulated_temp.h"

#define SERVO_DECLARATION
#include "servo_struct.h"
#undef SERVO_DECLARATION

void init_servo(struct servo_t *ptr, double *coef, int active_idx,
                int set_idx, int p_idx, int i_idx, int d_idx, int sat_idx,
                int mem_idx)
{
    int i, fir_len;

    servo = (struct servo_t **) realloc(servo, (++num_servo) *
                                        sizeof(struct servo_t *));
    i = num_servo - 1;
    servo[i] = ptr;

    for (fir_len = 0; coef[fir_len]; fir_len++);

    servo[i]->filt.coef = coef;

    circ_buf_init(&servo[i]->filt.buf, sizeof(double), fir_len);
    servo[i]->filt.val = 0.;

    circ_buf_init(&servo[i]->pid.resid, sizeof(double),
                  SERVO_RESIDUAL_DEPTH);

    servo[i]->pid.set_idx = set_idx;
    servo[i]->pid.p_idx = p_idx;
    servo[i]->pid.i_idx = i_idx;
    servo[i]->pid.d_idx = d_idx;
    servo[i]->pid.sat_idx = sat_idx;
    servo[i]->pid.mem_idx = mem_idx;
    servo[i]->pid.active_idx = active_idx;
    // assume by default that the servo is off, the the channel is alive
    servo[i]->pid.alive = ACTIVE;
}

void servo_init()
{
    servo = NULL;

#define SERVO_STRUCT_LIST
#include "servo_struct.h"
#undef SERVO_STRUCT_LIST
}

void do_servo_filter(int servo_index)
{
    int i, j;
    double filt_val;
    struct circ_buf_t *cb;

    i = servo_index;
    cb = &servo[i]->filt.buf;

    // FIR fiter of the buffer
    // 0 is the oldest and 1 is the newest
    filt_val =
        circ_buf_get(cb, double,
                     0) * servo[i]->filt.coef[circ_buf_len(cb) - 1];
    for (j = 0; j < (circ_buf_len(cb) - 1); j++) {
        filt_val +=
            circ_buf_get(cb, double, j + 1) * servo[i]->filt.coef[j];
    }

    servo[i]->filt.val = filt_val;
    //message(M_INFO, "%d %10.15f %10.15f", i, val, filt_val);
}

// IMPORTANT TODO: update this to use proper locks, shared params; before use
/* 
void do_driver_shutdown() {
  int i;

  for (i=0; i < num_servo; i++) {
    servo[i]->pid.active = INACTIVE;
  }

  return;
} */

void do_servo()
{
#ifndef __NO_SERVO__
    int i, j, rc;
    double residual, integral, derivative, output, saturation;

    rc = pthread_rwlock_rdlock(&params_rwlock);
    handle_errmsg("pthread_rwlock_rdlock()\n", rc);

    for (i = 0; i < num_servo; i++) {
        do_servo_filter(i);

        residual = param_val_curr[servo[i]->pid.set_idx] - servo[i]->filt.val;
        saturation = param_val_curr[servo[i]->pid.sat_idx];
        residual = residual > saturation ? saturation : residual;

        circ_buf_push(&servo[i]->pid.resid, residual);

        for (j = 0, integral = 0; j < SERVO_RESIDUAL_DEPTH; j++) {
            integral += circ_buf_get(&servo[i]->pid.resid, double, j) *
                pow(param_val_curr[servo[i]->pid.mem_idx], (double) j);
        }

        derivative =
            residual - circ_buf_get(&servo[i]->pid.resid, double, 1);

        output = param_val_curr[servo[i]->pid.p_idx] * residual +
            param_val_curr[servo[i]->pid.i_idx] * integral +
            param_val_curr[servo[i]->pid.d_idx] * derivative;

        output = output < DRV_LOWER ? DRV_LOWER : output;
        output = output > DRV_UPPER ? DRV_UPPER : output;

        servo[i]->pid.alive = param_val_curr[servo[i]->pid.active_idx];

        // if the temperature readout is malfunctioning (-1)
        // or should this be                         1?!!!!!!
        if (circ_buf_get(&servo[i]->filt.buf, double, 0) == -1) {
            servo[i]->pid.alive = INACTIVE;
            printf("Servo channel %d is dead\n", i);
            output = DRV_LOWER;
            // implement this
            //do_driver_shutdown(param_val_curr);
        }

        servo[i]->output = output;
        printf("%10.15g %10.15g\n", residual, output);

    }

    rc = pthread_rwlock_unlock(&params_rwlock);
    handle_errmsg("pthread_rwlock_unlock()\n", rc);
#endif
}

void *servo_thread(void *arg)
{
    double det1_now, previous_value, pprev, val_now;
    double time_now;

    FILE *outfile;
    outfile = fopen(SERVOTHREAD_OUTPUT, "w");
    struct timeval tv_now, tv_start;

    gettimeofday(&tv_start, NULL);

    while (1) {
        gettimeofday(&tv_now, NULL);
        time_now = (double) (tv_now.tv_sec - tv_start.tv_sec);
        time_now += ((double) (tv_now.tv_usec - tv_start.tv_usec)) / 1.e6;

        det1_now = GET_SERVO_TEMP(srv_detector1_idx, 0);
        previous_value = GET_SERVO_TEMP(srv_detector1_idx, 1);
        pprev = GET_SERVO_TEMP(srv_detector1_idx, 2);

        do_servo();
        val_now = servo[0]->output;

        fprintf(outfile, "%d %10.15f %10.5f %10.5f %10.5f %10.5f %10.5f\n",
                servo[0]->pid.alive, time_now, det1_now, previous_value, pprev, val_now, servo[0]->filt.val);

        if(servo[0]->pid.alive != INACTIVE) {
            current_power = val_now / 4096.;
        }

        fflush(outfile);
        usleep(SERVO_LOOP_WAIT);
    }

    fclose(outfile);
}
