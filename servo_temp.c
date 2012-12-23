#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "simulated_temp.h"
#include "read_temp.h"
#include "servo_temp.h"

void servo_temp_init () {
    printf("initializing servo thread\n");
    circ_buf_init(&temperature_buf, sizeof(double), TEMPERATURE_BUFLEN);
}

void *servo_temp_thread(void *arg) {
    double current_value, filtered_value;

    while (1) {
        usleep(100000);
        current_value = get_temperature("detector1");
        circ_buf_push(&temperature_buf, current_value);
        filtered_value = circ_buf_get(&temperature_buf, double, 1);
        printf("servo: %10.5f %10.5f %10.5f\n",
               current_temperature, current_value, filtered_value);
    }
}
