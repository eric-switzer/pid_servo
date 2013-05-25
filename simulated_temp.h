#ifndef SIMULATED_TEMP_H
#define SIMULATED_TEMP_H

double current_controller;
double current_temperature;
double current_reading;

#define SIMTEMPTHREAD_OUTPUT "temp_simulation.dat"

void simulate_temp_init();
void *simulate_temp_thread(void *arg);

#define SIM_RATE  100
#define RATE_COEF (1.e-6)
#define BATH_TEMP (100.)
#define POWER_SCALE (1.)

#endif
