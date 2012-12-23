#ifndef READ_TEMP_H
#define READ_TEMP_H

// define the shared temperature hash
//-----------------------------------------------------------------------------
#include "uthash.h"
#include "circular_buffer.h"

#define TEMP_ENTRY_KEY_LEN 256
#define TEMPERATURE_BUFLEN 1000
typedef struct temperature_entry_t {
    char key[TEMP_ENTRY_KEY_LEN];
    struct circ_buf_t temperature_buf;
    UT_hash_handle hh;
} temperature_entry_t;

void add_temperature(const char *name);

// here look_back recalls the circular buffer look_back pushes ago
double get_temperature(const char *name, int look_back);

// this can only push temperatures onto the top of the buffer
void push_temperature(const char *name, double val);

void list_temperatures();

// main readout functions
//-----------------------------------------------------------------------------

#define READTHREAD_OUTPUT "readthread.dat"
void read_temp_init();
void *read_temp_thread(void *arg);

#endif
