#ifndef SERVO_TEMP_H
#define SERVO_TEMP_H

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

void servo_temp_init();
void *servo_temp_thread(void *arg);

#endif
