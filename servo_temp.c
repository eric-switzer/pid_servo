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

    while (1) {
        current_value = get_temperature("detector1", 1);
        previous_value = get_temperature("detector1", 2);

        fflush(outfile);

        usleep(100000);
    }
    fclose(outfile);
}
