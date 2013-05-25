#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "simulated_temp.h"
#include <sys/time.h>
/*
For a heater on a mass with capacity C and link G to bath T_b,
C dT/dt = P - G(T-T_b)
T_{i+1} = T_i + \Delta t / C (P - G(T_i - T_b))
renormalize the power to tilde P so that
T_{i+1} = T_i + alpha (tilde P - (T_i - T_b))
*/

// open the device and build the calibration table
void simulate_temp_init()
{
    printf("initializing sim thread\n");
}

void *simulate_temp_thread(void *arg)
{
    FILE *outfile;
    outfile = fopen(SIMTEMPTHREAD_OUTPUT, "w");
    double time_now;
    double previous_temperature = BATH_TEMP;
    struct timeval tv_start, tv_now;

    gettimeofday(&tv_start, NULL);

    current_temperature = BATH_TEMP;

    while (1) {
        gettimeofday(&tv_now, NULL);
        time_now = (double) (tv_now.tv_sec - tv_start.tv_sec);
        time_now += ((double) (tv_now.tv_usec - tv_start.tv_usec)) / 1.e6;

        previous_temperature = current_temperature;

        current_temperature = current_power / POWER_SCALE;
        current_temperature +=
            cos(time_now / 2. / M_PI) / POWER_SCALE / 1.;
        current_temperature -= previous_temperature - BATH_TEMP;
        current_temperature *= RATE_COEF;
        current_temperature += previous_temperature;

        // simulate the measured temperature; could add noise here
        current_reading = current_temperature;

        fprintf(outfile, "t=%10.15f P=%10.5f T=%10.5f Tm=%10.5f\n",
                time_now, current_power, current_temperature,
                current_reading);

        fflush(outfile);

        usleep(10000);
    }

    fclose(outfile);
}
