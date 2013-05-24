#ifndef SERVO_H
#define SERVO_H

#define SERVO_LOOP_WAIT         1000000
#define SERVO_RESIDUAL_DEPTH    1000
#define DRV_LOWER               0
#define DRV_UPPER               4096
#define SERVOTHREAD_OUTPUT      "servo.dat"

#include "circular_buffer.h"

struct filter_t {
  double *coef;
  struct circ_buf_t buf;
  double val;
};

struct pid_servo_t {
  struct circ_buf_t resid;
  int active_idx;
  int set_idx;
  int p_idx;
  int i_idx;
  int d_idx;
  int sat_idx;
  int mem_idx;
  int alive;
};

struct servo_t {
  double temp;
  struct filter_t filt;
  struct pid_servo_t pid;
  double output;
};

int num_servo;
struct servo_t **servo;

#define INIT_FILTER_COEFF(var_name, ...) double var_name[] = {__VA_ARGS__, 0};

#define SERVO_DECLARE(var_name, ...)\
  struct servo_t var_name;

#define SERVO_LIST(var_name, filter_name, active_idx, set_idx, p_idx, i_idx, \
                   d_idx, sat_idx, mem_idx) \
  init_servo(&var_name, filter_name, active_idx, set_idx, p_idx, i_idx, \
             d_idx, sat_idx, mem_idx);

#ifndef NULL_DEFINE
  #define NULL_DEFINE(...)
#endif

#define GET_SERVO_TEMP(servo_id, lookback)\
  circ_buf_get(&servo[servo_id]->filt.buf, double, lookback);

#define PUSH_SERVO_TEMP(servo_id, val)\
  circ_buf_push(&servo[servo_id]->filt.buf, val);

#define ACTIVE     1
#define INACTIVE   0

void servo_init();

void init_servo(struct servo_t *ptr, double *coef, int active_idx,
                int set_idx, int p_idx, int i_idx, int d_idx, int sat_idx,
                int mem_idx);

void do_servo_filter(int servo_index);

void do_servo();

void *servo_thread(void *arg);

#endif

// This "environment" just declares the servo objects
#ifdef SERVO_DECLARATION
  #undef SERVO_INIT
  #undef MAKE_FILTER_COEFF
  #define SERVO_INIT SERVO_DECLARE
  #define MAKE_FILTER_COEFF INIT_FILTER_COEFF
// Initialize the servo objects
#elif defined SERVO_STRUCT_LIST
  #undef SERVO_INIT
  #undef MAKE_FILTER_COEFF
  #define SERVO_INIT SERVO_LIST
  #define MAKE_FILTER_COEFF NULL_DEFINE
// Do nothing with the servo structure
#else
  #undef SERVO_INIT
  #undef MAKE_FILTER_COEFF
  #define SERVO_INIT NULL_DEFINE
  #define MAKE_FILTER_COEFF NULL_DEFINE
#endif
