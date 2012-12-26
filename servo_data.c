#include <stdlib.h>
#include <stdio.h>
#include "servo_data.h"

//-----------------------------------------------------------------------------
// define the temperature lookup functions
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

