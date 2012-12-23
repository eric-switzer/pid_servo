#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "redis_control.h"

// define the shared rediscontrol hash
//-----------------------------------------------------------------------------
rediscontrol_entry_t *rediscontrol_entries=NULL, *rediscontrol_entry;

void add_rediscontrol(const char *name, double val) {
    if ( (rediscontrol_entry =
         (rediscontrol_entry_t*)
         malloc(sizeof(rediscontrol_entry_t))) == NULL) exit(-1);

    strncpy(rediscontrol_entry->key, name, REDISCONTROL_ENTRY_KEY_LEN);
    rediscontrol_entry->val = val;
    HASH_ADD_STR(rediscontrol_entries, key, rediscontrol_entry);

    return;
}

double get_rediscontrol(const char *name) {
    HASH_FIND_STR(rediscontrol_entries, name, rediscontrol_entry);
    if (rediscontrol_entry) {
        printf("returning %s (param_val %10.5g)\n",
               rediscontrol_entry->key, rediscontrol_entry->val);
    } else {
        printf("failed to find %s\n", name);
    }

    return rediscontrol_entry->val;
}

void set_rediscontrol(const char *name, double val) {
    HASH_FIND_STR(rediscontrol_entries, name, rediscontrol_entry);
    if (rediscontrol_entry) {
        printf("setting %s (param_val %10.5g -> %10.5g)\n",
               rediscontrol_entry->key, rediscontrol_entry->val, val);

    } else {
        printf("failed to find %s\n", name);
    }

    rediscontrol_entry->val = val;
    return;
}

void list_rediscontrols() {
    struct rediscontrol_entry_t *s, *tmp;

    HASH_ITER(hh, rediscontrol_entries, s, tmp) {
        printf("%s: %10.5g\n", s->key, s->val);
    }

    return;
}

// open the device and build the calibration table
void redis_control_init () {
    printf("initializing redis control thread\n");
}

void *redis_control_thread(void *arg) {
    while (1) {
        usleep(10000);
    }
}
