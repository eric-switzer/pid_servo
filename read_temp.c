#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

// define the shared temperature hash
// TODO: improve error handling for failed keys
//-----------------------------------------------------------------------------
temperature_entry_t *temperature_entries=NULL, *temperature_entry;

void add_temperature(const char *name) {
    if ( (temperature_entry =
         (temperature_entry_t*)
         malloc(sizeof(temperature_entry_t))) == NULL) exit(-1);

    strncpy(temperature_entry->key, name, TEMP_ENTRY_KEY_LEN);

    circ_buf_init(&temperature_entry->temperature_buf,
                  sizeof(double), TEMPERATURE_BUFLEN);

    HASH_ADD_STR(temperature_entries, key, temperature_entry);

    return;
}

double get_temperature(const char *name, int look_back) {
    HASH_FIND_STR(temperature_entries, name, temperature_entry);
    if (!temperature_entry) printf("failed to find %s\n", name);

    return circ_buf_get(&temperature_entry->temperature_buf,
                        double, look_back);
}

void push_temperature(const char *name, double val) {
    HASH_FIND_STR(temperature_entries, name, temperature_entry);
    if (!temperature_entry) printf("failed to find %s\n", name);

    circ_buf_push(&temperature_entry->temperature_buf, val);
    return;
}

void list_temperatures() {
    struct temperature_entry_t *s, *tmp;
    double val;

    HASH_ITER(hh, temperature_entries, s, tmp) {
        val = circ_buf_get(&s->temperature_buf,
                           double, 1);
        printf("%s: %10.5g\n", s->key, val);
    }

    return;
}

// open the device and build the calibration table
void read_temp_init () {
    printf("initializing read thread\n");

    // register temperatures used internally to amcp
    add_temperature("detector1");
    add_temperature("detector2");
    add_temperature("detector3");
}

void *read_temp_thread(void *arg) {
    FILE *outfile;
    outfile=fopen(READTHREAD_OUTPUT, "w");
    struct timeval tv;

    while (1) {
        gettimeofday(&tv,NULL);
        push_temperature("detector1", current_reading);
        fprintf(outfile, "%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, current_reading);
        fflush(outfile);
        usleep(10000);
    }
    fclose(outfile);
}
