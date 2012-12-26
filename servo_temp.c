#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "servo_data.h"
#include "servo_temp.h"

void servo_temp_init () {
    printf("initializing servo thread; available temperatures\n");
    list_temperatures();
}

void *servo_temp_thread(void *arg) {
    double current_value, previous_value;
    FILE *outfile;
    outfile=fopen(SERVOTHREAD_OUTPUT, "w");
    struct timeval tv;

    while (1) {
        gettimeofday(&tv,NULL);
        current_value = get_temperature("detector1", 1);
        previous_value = get_temperature("detector1", 2);

        fprintf(outfile, "%ld.%ld %10.5f %10.5f %10.5f\n",
                         tv.tv_sec, tv.tv_usec,
                         current_temperature, current_value, previous_value);

        fflush(outfile);

        usleep(100000);
    }
    fclose(outfile);
}
