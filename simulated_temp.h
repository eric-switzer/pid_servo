#ifndef SIMULATED_TEMP_H
#define SIMULATED_TEMP_H

double current_power;
double current_temperature;
double current_reading;

#define SIMTEMPTHREAD_OUTPUT "temp_simulation.dat"

void simulate_temp_init();
void *simulate_temp_thread(void *arg);

#define RATE_COEF (0.01)
#define BATH_TEMP (100.)
#define POWER_SCALE (1.)

#endif
