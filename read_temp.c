#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

// ADD THIS FOR THE SERVO
#include "servo.h"
#include "servo_struct.h"

double rand_normal(void)
{
    static double v, fac;
    static int phase = 0;
    double mag, rand_out, u;

    if (phase) {
        rand_out = v * fac;
    } else {
        do {
            u = 2. * (double) rand() / RAND_MAX - 1.;
            v = 2. * (double) rand() / RAND_MAX - 1.;
            mag = u * u + v * v;
        } while (mag >= 1);

        fac = sqrt(-2. * log(mag) / mag);
        rand_out = u * fac;
    }

    phase = 1 - phase;

    return rand_out;
}

void read_temp_init()
{
    printf("initializing read thread\n");
}

void *read_temp_thread(void *arg)
{
    FILE *outfile;
    outfile = fopen(READTHREAD_OUTPUT, "w");
    struct timeval tv_now, tv_start;
    double time_now;
    double temp_value = 0.;

    gettimeofday(&tv_start, NULL);

    while (1) {
        gettimeofday(&tv_now, NULL);
        time_now = (double) (tv_now.tv_sec - tv_start.tv_sec);
        time_now += ((double) (tv_now.tv_usec - tv_start.tv_usec)) / 1.e6;

        usleep(READ_RATE);
        temp_value = current_reading + rand_normal() / 20.;

        // ADD THIS FOR THE SERVO
        PUSH_SERVO_TEMP(srv_detector1_idx, temp_value);

        fprintf(outfile, "%10.15f %10.15g %10.15g %10.15g\n",
                time_now, temp_value, current_reading, current_controller);

        fflush(outfile);
    }
    fclose(outfile);
}
