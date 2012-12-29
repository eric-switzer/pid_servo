#ifndef CONTROL_H
#define CONTROL_H

#define REDIS_HOST "127.0.0.1"
#define REDIS_HOST_PORT 6379
#define REDIS_HK_CHANNEL "housekeeping"
#define REDIS_HK_ACK_CHANNEL "housekeeping_ack"

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
  int cmdnum;
};

void control_init();
void *command_thread(void *arg);
void *control_thread(void *arg);
int num_ctrl_cmd_param();
int ctrl_cmd_to_index(const char *field);

#endif