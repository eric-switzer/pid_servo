#ifndef REDIS_CONTROL_H
#define REDIS_CONTROL_H

// define the shared rediscontrol hash
//-----------------------------------------------------------------------------
#include "uthash.h"

#define REDISCONTROL_ENTRY_KEY_LEN 256
typedef struct rediscontrol_entry_t {
    char key[REDISCONTROL_ENTRY_KEY_LEN];
    double val;
    UT_hash_handle hh;
} rediscontrol_entry_t;

void add_rediscontrol(const char *name, double val);
double get_rediscontrol(const char *name);
void set_rediscontrol(const char *name, double val);
void list_rediscontrols();

// main control functions
//-----------------------------------------------------------------------------

void redis_control_init();
void *redis_control_thread(void *arg);

#endif
