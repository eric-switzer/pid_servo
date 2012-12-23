#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "simulated_temp.h"
#include "read_temp.h"

// define the shared temperature hash
//-----------------------------------------------------------------------------
temperature_entry_t *temperature_entries=NULL, *temperature_entry;

void add_temperature(const char *name, double val) {
    if ( (temperature_entry =
         (temperature_entry_t*)
         malloc(sizeof(temperature_entry_t))) == NULL) exit(-1);

    strncpy(temperature_entry->key, name, TEMP_ENTRY_KEY_LEN);
    temperature_entry->val = val;
    HASH_ADD_STR(temperature_entries, key, temperature_entry);

    return;
}

double get_temperature(const char *name) {
    HASH_FIND_STR(temperature_entries, name, temperature_entry);
    if (temperature_entry) {
        printf("returning %s (param_val %10.5g)\n",
               temperature_entry->key, temperature_entry->val);
    } else {
        printf("failed to find %s\n", name);
    }

    return temperature_entry->val;
}

void set_temperature(const char *name, double val) {
    HASH_FIND_STR(temperature_entries, name, temperature_entry);
    if (temperature_entry) {
        printf("setting %s (param_val %10.5g -> %10.5g)\n",
               temperature_entry->key, temperature_entry->val, val);

    } else {
        printf("failed to find %s\n", name);
    }

    temperature_entry->val = val;
    return;
}

void list_temperatures() {
    struct temperature_entry_t *s, *tmp;

    HASH_ITER(hh, temperature_entries, s, tmp) {
        printf("%s: %10.5g\n", s->key, s->val);
    }

    return;
}

// open the device and build the calibration table
void read_temp_init () {
    printf("initializing read thread\n");

    // register temperatures used internally to amcp
    add_temperature("detector1", 0.);
    add_temperature("detector2", 0.);
    add_temperature("detector3", 0.);

}

void *read_temp_thread(void *arg) {
    FILE *outfile;
    outfile=fopen(READTHREAD_OUTPUT, "w");
    struct timeval tv;

    while (1) {
        gettimeofday(&tv,NULL);
        set_temperature("detector1", current_reading);
        fprintf(outfile, "%ld.%ld %10.5f\n", tv.tv_sec, tv.tv_usec, current_reading);
        fflush(outfile);
        usleep(10000);
    }
    fclose(outfile);
}
