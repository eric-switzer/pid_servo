#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "redis_control.h"
#include "servo_data.h"

// open the device and build the calibration table
void redis_control_init () {
    printf("initializing redis control thread\n");
}

void *redis_control_thread(void *arg) {
    while (1) {
        usleep(10000);
    }
}
