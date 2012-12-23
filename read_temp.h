#ifndef READ_TEMP_H
#define READ_TEMP_H

// define the shared temperature hash
//-----------------------------------------------------------------------------
#include "uthash.h"

# define TEMP_ENTRY_KEY_LEN 256
typedef struct temperature_entry_t {
    char key[TEMP_ENTRY_KEY_LEN];
    double val;
    UT_hash_handle hh;
} temperature_entry_t;

void add_temperature(const char *name, double val);
double get_temperature(const char *name);
void set_temperature(const char *name, double val);

// main readout functions
//-----------------------------------------------------------------------------

void read_temp_init();
void *read_temp_thread(void *arg);

#endif
