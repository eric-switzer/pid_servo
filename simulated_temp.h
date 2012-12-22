#ifndef SIMULATED_TEMP_H
#define SIMULATED_TEMP_H

double current_power;
double current_temperature;
double current_reading;

void simulate_temp_init();
void *simulate_temp_thread(void *arg);

#endif
