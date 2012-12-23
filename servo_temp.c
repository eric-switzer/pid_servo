#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"
#include "servo_temp.h"

void servo_temp_init () {
    printf("initializing servo thread; available temperatures\n");

    list_temperatures();

    circ_buf_init(&temperature_buf, sizeof(double), TEMPERATURE_BUFLEN);
}

void *servo_temp_thread(void *arg) {
    double current_value, filtered_value;
    FILE *outfile;
    outfile=fopen(SERVOTHREAD_OUTPUT, "w");
    struct timeval tv;

    while (1) {
        gettimeofday(&tv,NULL);
        current_value = get_temperature("detector1");
        circ_buf_push(&temperature_buf, current_value);
        filtered_value = circ_buf_get(&temperature_buf, double, 1);

        fprintf(outfile, "%ld.%ld %10.5f %10.5f %10.5f\n",
                         tv.tv_sec, tv.tv_usec,
                         current_temperature, current_value, filtered_value);

        fflush(outfile);

        usleep(100000);
    }
    fclose(outfile);
}
