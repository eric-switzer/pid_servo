#ifndef CONTROL_H
#define CONTROL_H
#include<pthread.h>

#define REDIS_HOST "127.0.0.1"
#define REDIS_HOST_PORT 6379
#define REDIS_HK_CHANNEL "housekeeping"
#define REDIS_HK_ACK_CHANNEL "commanding_ack"

#define FIELD_LEN 256
#define UNITS_LEN 256
#define CTRL_CMD_TYPE_LEN           32
#define CTRL_CMD_SYS_LEN            32

struct ctrl_cmd_param_t {
    char name[FIELD_LEN];
    char type[CTRL_CMD_TYPE_LEN];
    char sys[CTRL_CMD_SYS_LEN];
    char units[UNITS_LEN];
    double min;
    double max;
    double default_val;
};

#define END_OF_CTRL_CMD_PARAM       {"", "", "", "", -1, -1, -1}
#define END_OF_CTRL_SYS             ""

// current parameter values (globally available, thread locked)
pthread_rwlock_t params_rwlock;
double *param_val_curr;
void handle_errmsg(char *string, int rc);

void control_init();
void *command_thread(void *arg);
void *control_thread(void *arg);
int num_ctrl_cmd_param();
int ctrl_cmd_to_index(const char *field);

#endif
