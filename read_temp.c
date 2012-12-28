#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

// open the device and build the calibration table
void read_temp_init () {
    printf("initializing read thread\n");
}

void *read_temp_thread(void *arg) {
    FILE *outfile;
    outfile=fopen(READTHREAD_OUTPUT, "w");
    struct timeval tv;

    while (1) {
        gettimeofday(&tv,NULL);
        //push_temperature("detector1", current_reading);
        fprintf(outfile, "%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, current_reading);
        fflush(outfile);
        usleep(10000);
    }
    fclose(outfile);
}
