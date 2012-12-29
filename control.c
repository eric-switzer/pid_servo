#include <act_util.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "control_struct.h"
#include "extern.h"
#include "frame.h"
#include "kuka.h"
#include "pointing.h"
#include "servo.h"
#include "window.h"
#include "control.h"

int max_ctrl_cmd_rate;
int fd_sync;
double *param_val_curr;
// flag: external command received or amcp request
int *cmd_change = NULL;
int *amcp_change = NULL;

void open_sync_com();
void message_handler(redisAsyncContext *c, void *reply, void *privdata);
void write_to_sync(const char *cmd);
void read_from_sync(char *buf);
void command_sync(int index);

int num_ctrl_cmd_param() {
  static int num = -1;

  if (num < 0)
    for (num = 0; strlen(ctrl_cmd_param[num].name); num++);

  return num;
}

// Find the range of command numbers in the structure.  Note that this is
// different than num_ctrl_cmd_param:  for control of multiple systems,
// there could be several hundred command numbers, but only a subset of those
// that are particular to amcp are in control_struct.c.
int ctrl_cmd_param_max_cmdnum() {
  static int num = -1;
  int i, j;

  if (num < 0) {
    for (i = 0; strlen(ctrl_cmd_param[i].name); i++) {
      j = ctrl_cmd_param[i].cmdnum;
      if (num < j)
        num = j;
    }
  }
  return num;
}

// Find the index of the control param with a given name.
int ctrl_cmd_to_index(const char *field) {
  int i;
  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    if (!strcmp(field, ctrl_cmd_param[i].name))
      return i;
  }
  return -1;
}

// Find the number of system types.
int num_ctrl_sys() {
  static int num = -1;

  if (num < 0)
    for (num = 0; strlen(ctrl_sys[num]); num++);

  return num;
}

// Find the system type given the index of ctrl_cmd_param.
int ctrl_cmd_sys(int index) {
  int i, j;
  static int *mapping = NULL;

  if (mapping == NULL) {
    mapping = (int *)act_malloc("mapping", num_ctrl_cmd_param() * sizeof(int));
    for (i = 0; i < num_ctrl_cmd_param(); i++) {
      mapping[i] = -1;
      for (j = 0; j < num_ctrl_sys(); j++) {
        if (!strcmp(ctrl_sys[j], ctrl_cmd_param[i].sys)) {
          mapping[i] = j;
          break;
        }
      }
    }
  }

  return mapping[index];
}

// Checks to see if the command is an input_select type.
char ctrl_cmd_is_select(int ctrl_index) {
  int i;
  static char *mapping = NULL;

  if (mapping == NULL) {
    mapping = (char *)act_malloc("mapping", num_ctrl_cmd_param() *
                                            sizeof(char));
    for (i = 0; i < num_ctrl_cmd_param(); i++) {
      if (!strcmp(ctrl_cmd_param[i].type, CTRL_CMD_SELECT_NAME))
        mapping[i] = 1;
      else
        mapping[i] = 0;
    }
  }

  return mapping[ctrl_index];
}

int ctrl_cmd_to_sensorayio_write_map(int ctrl_index) {
  int i, j;
  static int *mapping = NULL;

  if (mapping == NULL) {
    mapping = (int *)act_malloc("mapping", num_ctrl_cmd_param() * sizeof(int));
    for (i = 0; i < num_ctrl_cmd_param(); i++) {
      mapping[i] = -1;
      for (j = 0; j < num_sensorayio_write_map(); j++) {
        if (!strcmp(sensorayio_write_map[j].ctrl_name,
                    ctrl_cmd_param[i].name)) {
          mapping[i] = j;
          break;
        }
      }
    }
  }

  return mapping[ctrl_index];
}

int ctrl_cmd_to_kuka_dn_write_map(int ctrl_index) {
  int i, j;
  static int *mapping = NULL;

  if (mapping == NULL) {
    mapping = (int *)act_malloc("mapping", num_ctrl_cmd_param() * sizeof(int));
    for (i = 0; i < num_ctrl_cmd_param(); i++) {
      mapping[i] = -1;
      for (j = 0; j < num_kuka_dn_write_map; j++) {
        if (!strcmp(kuka_dn_write_map[j].ctrl_name, ctrl_cmd_param[i].name)) {
          mapping[i] = j;
          break;
        }
      }
    }
  }

  return mapping[ctrl_index];
}

void open_sync_com() {
  #ifndef __NO_SYNC_BOX__
    struct termios tio;

    message(M_START, "Opening %s for communication with sync box.",
                     SYNC_DEVICE);

    if ((fd_sync = open(SYNC_DEVICE, O_RDWR | O_NOCTTY)) < 0)
      message(M_FATAL, "Couldn't open %s for communication with sync "
                       "box.", SYNC_DEVICE);

    // Set up the TTY parameters.  We leave VTIME and VLEN = 0 so that the
    // read is non-blocking.
    memset((char *)&tio, 0, sizeof(struct termios));
    tio.c_cflag = B9600 | CSIZE | CREAD | CLOCAL;
    tio.c_iflag = IGNPAR | ICRNL;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VEOF] = 4;

    tcflush(fd_sync, TCIFLUSH);
    tcsetattr(fd_sync, TCSANOW, &tio);
  #else
    message(M_START, "Not opening COM to sync box due to __NO_SYNC_BOX__ "
                     "flag.");
  #endif
}


void control_init() {
  redisContext *redis;
  redisReply *reply;
  int i, j, n, index, bit;
  int push_default = 1;

  //TODO REVERT ME
  //if (!done_kuka_init)
  //  message(M_FATAL, "Subroutine kuka_init() must be called before"
  //                   "control_init().");

  open_sync_com();

  // Allocate the arrays holding the values of the parameter values.
  param_val_curr = (double *)act_malloc("param_val_curr", num_ctrl_cmd_param() *
                                                          sizeof(double));

  if (cmd_change == NULL) {
    cmd_change = (int *)act_calloc("cmd_change", num_ctrl_cmd_param(),
                                   sizeof(int));
  }
  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    cmd_change[i] = 0;
  }

  if (amcp_change == NULL)
    amcp_change = (int *)act_calloc("amcp_change", num_ctrl_cmd_param(),
                                    sizeof(int));
  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    amcp_change[i] = 0;
  }

  // Initialise the parameter values to their defaults, in case
  // redis fails (fall to safe state)
  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    param_val_curr[i] = ctrl_cmd_param[i].default_val;
    if (ctrl_cmd_sys(i) == CTRL_SYS_POINTING) {
      change_point_info(i, param_val_curr[i]);
    }
  }

  redis = redisConnect(REDIS_HOST, REDIS_HOST_PORT);
  if (redis->err) {
    fprintf(stderr, "Connection error: %s\n", redis->errstr);
    exit(EXIT_FAILURE);
  }

  if (push_default) {
    message(M_START, "Writing default command values to the server.");
  } else {
    message(M_START, "Getting current command values.");
  }

  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    if (push_default) {
      reply = redisCommand(redis, "SET %s %10.15g",
                           ctrl_cmd_param[i].name,
                           ctrl_cmd_param[i].default_val);

      message(M_START, "REDIS: set %s %10.15g returns %s",
              ctrl_cmd_param[i].name,
              ctrl_cmd_param[i].default_val,
              reply->str);

      freeReplyObject(reply);
    } else {
      reply = redisCommand(redis, "GET %s", ctrl_cmd_param[i].name);
      if ( reply->type == REDIS_REPLY_ERROR ) {
        message(M_WARN, "Error: %s\n", reply->str);
      } else if ( reply->type != REDIS_REPLY_STRING ) {
        message(M_WARN, "Unexpected type/no value: %d\n", reply->type);
      } else {
        sscanf(reply->str, "%lf", &param_val_curr[i]);
        message(M_INFO, "Result for %s: %10.15g\n",
                ctrl_cmd_param[i].name,
                param_val_curr[i]);
      }
      freeReplyObject(reply);
    }

    // TODO: REVERT ME!!!!
    // dispatch the value
    /* switch (ctrl_cmd_sys(i)) {
      case CTRL_SYS_POINTING:
        change_point_info(i, param_val_curr[i]);
        break;
      case CTRL_SYS_KUKA_DN:
        if ((j = ctrl_cmd_to_kuka_dn_write_map(i)) >= 0) {
          index = kuka_dn_bitmap[kuka_dn_write_map[j].index]->index;
          bit = kuka_dn_bitmap[kuka_dn_write_map[j].index]->bit;
          k = param_val_curr[i] ? 1 : 0;
          #ifndef __NO_KUKA__
            kuka_dn_io[index] &= ~(1 << bit);
            kuka_dn_io[index] |= (k & 1) << bit;
          #endif
        }
        break;
    }*/
  }
  message(M_START, "Finished retrieving current command values.");

  // Set up a circular buffer for telling Sensoray its new values.
  circ_buf_init(&new_sensoray_write_buf, sizeof(struct new_sensoray_write_t),
                NEW_SENSORAY_WRITE_BUFLEN);

  // Set up a circular buffer for telling DeviceNet its new values.
  circ_buf_init(&new_kuka_dn_write_buf, sizeof(struct new_kuka_dn_write_t),
                NEW_KUKA_DN_WRITE_BUFLEN);

  // Set up a circular buffer for giving the pointing system commands.
  circ_buf_init(&new_point_cmd_buf, sizeof(struct new_point_cmd_t),
                NEW_POINT_CMD_BUFLEN);

  message(M_START, "Starting redis command subscriber");

  // Start the subscriber thread here? or in amcp.c
  //pthread_create(&command_loop, NULL, command_thread, NULL);
}

void message_handler(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;
  int i, k, cmdindex;
  double cmdval;
  char *sp;
  struct new_sensoray_write_t new_sensoray_write;
  struct new_kuka_dn_write_t new_kuka_dn_write;
  struct new_point_cmd_t new_point_cmd;

  if (reply == NULL) return;

  if (r->type == REDIS_REPLY_ARRAY) {
    if (!strcmp(r->element[1]->str, REDIS_HK_CHANNEL)) {
      if ( r->element[2]->str != NULL ) {
        sp = strchr(r->element[2]->str, ' ');
        if ( sp != NULL ) {
          *sp++ = '\0';
          sscanf(sp, "%lf", &cmdval);
          cmdindex = ctrl_cmd_to_index(r->element[2]->str);
          if (cmdindex > 0) {
            param_val_curr[cmdindex] = cmdval;
            cmd_change[cmdindex] = 1;
            message(M_INFO, "Received command %s (idx=%d) with value %g.\n",
                    ctrl_cmd_param[cmdindex].name,
                    cmdindex,
                    param_val_curr[cmdindex]);

            i = cmdindex;
            switch (ctrl_cmd_sys(i)) {
              case CTRL_SYS_POINTING:
                message(M_INFO, "new pointing command");
                new_point_cmd.cmd_index = i;
                new_point_cmd.val = param_val_curr[i];
                if (!circ_buf_full(&new_point_cmd_buf)) {
                  message(M_INFO, "pushed new pointing command to buffer");
                  circ_buf_push(&new_point_cmd_buf, new_point_cmd);
                } else {
                  message(M_WARN, "Circular buffer for new_point_cmd full.");
                }
                break;
              case CTRL_SYS_ABOB:
                // Figure out where this value will go.
                if ((k = ctrl_cmd_to_sensorayio_write_map(i)) >= 0) {
                  new_sensoray_write.sensorayio_index = write_map_to_sensoray(k);
                  new_sensoray_write.val = param_val_curr[i];
                   // Push to circular buffer.
                  if (!circ_buf_full(&new_sensoray_write_buf)) {
                    circ_buf_push(&new_sensoray_write_buf, new_sensoray_write);
                  } else {
                    message(M_WARN, "Circular buffer new_sensoray_write_buf full.");
                  }
                } else {
                  message(M_WARN, "Did not route command %s to Sensoray because it "
                                  "does not have an entry in the "
                                  "sensorayio_write_map structure.",
                                  ctrl_cmd_param[i].name);
                }
                break;
              case CTRL_SYS_KUKA_DN:
                // Figure out where this value will go.
                message(M_INFO, "new kuka dn command");
                if ((k = ctrl_cmd_to_kuka_dn_write_map(i)) >= 0) {
                  new_kuka_dn_write.bitmap =
                    *(kuka_dn_bitmap[kuka_dn_write_map[k].index]);
                  new_kuka_dn_write.val = param_val_curr[i] ? 1 : 0;
                   // Push to circular buffer.
                  if (!circ_buf_full(&new_kuka_dn_write_buf)) {
                    message(M_INFO, "push to dn command buffer");
                    circ_buf_push(&new_kuka_dn_write_buf, new_kuka_dn_write);
                  } else {
                    message(M_WARN, "Circular buffer new_kuka_dn_write_buf "
                                    "full.");
                  }
                } else {
                  message(M_WARN, "Did not route command %s to DeviceNet "
                                  "because it does not have an entry in the "
                                  "kuka_dn_write_map structure.",
                                  ctrl_cmd_param[i].name);
                }
                break;
              case CTRL_SYS_SYNC:
                command_sync(i);
                break;
              case CTRL_SYS_INT_VARIABLE:
                if (i == window_open_idx)
                  open_window_cover();
                if (i == window_close_idx)
                  close_window_cover();
                // So far, the servo.c portion takes care of these above.
                //message(M_WARN, "Did not route command %s because this "
                //                "internal variable has not yet been "
                //                "implemented.", ctrl_cmd_param[i].name);
                break;
            }
          } else {
            message(M_WARN, "Unknown system variable received: %s\n",
                    r->element[2]->str);
          }
        } else {
          message(M_WARN, "Malformed command received: %s", r->element[2]->str);
        }
      }
    }
    if (!strcmp(r->element[1]->str, "messages")) {
      message(M_INFO, "received a log message: handle this?");
    }
  }

  return;
}

void *command_thread(void *arg) {
  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  message(M_INFO, "starting sub conn");
  redisAsyncContext *c = redisAsyncConnect(REDIS_HOST, REDIS_HOST_PORT);
  if (c->err) {
    message(M_FATAL, "REDIS not connected: %s", c->errstr);
  }

  message(M_INFO, "starting lib event");
  redisLibeventAttach(c, base);
  message(M_INFO, "send subscribe");
  redisAsyncCommand(c, message_handler, NULL, "SUBSCRIBE housekeeping");
  message(M_INFO, "dispatch handler");
  event_base_dispatch(base);
  return 0;
}

void *control_thread(void *arg) {
  char send_amcp_changes;
  int i, last_frame_pos;

  redisContext *redis;
  redisReply *reply;

  redis = redisConnect(REDIS_HOST, REDIS_HOST_PORT);
  if (redis->err) {
    message(M_FATAL, "Redis connection error: %s", redis->errstr);
    exit(EXIT_FAILURE);
  }

  // Run control loop at 400 Hz.
  last_frame_pos = 0;
  send_amcp_changes = 0;
  for (;;) {
    if (last_frame_pos != get_frame_pos()) {
      // Check to see if we should register changes amcp made (once per
      // second).
      if (last_frame_pos > get_frame_pos())
        send_amcp_changes = 1;

      // Do servoing.
      //do_servo(param_val_curr);

      // Check to see if anything needs to be updated.
      for (i = 0; i < num_ctrl_cmd_param(); i++) {
        if (cmd_change[i]) {
          // Push the values of requested change values back to the server.
          reply = redisCommand(redis, "SET %s %10.15g",
                               ctrl_cmd_param[i].name,
                               param_val_curr[i]);
          freeReplyObject(reply);

          reply = redisCommand(redis, "PUBLISH %s %s",
                               REDIS_HK_ACK_CHANNEL,
                               ctrl_cmd_param[i].name);
          freeReplyObject(reply);

          message(M_INFO, "Ack command: %s->%g.",
                 ctrl_cmd_param[i].name, param_val_curr[i]);

          // reset and also block a change by amcp if made at the same time
          if (amcp_change[i]) {
            amcp_change[i] = 0;
          }
        } else if (amcp_change[i] && send_amcp_changes) {
          // Every second (when send_amcp_changes = 1), send any changes not
          // requested by the interface_server, but which have been changed by
          // amcp (e.g., during autocycling).
          reply = redisCommand(redis, "SET %s %10.15g",
                               ctrl_cmd_param[i].name,
                               param_val_curr[i]);
          freeReplyObject(reply);

          reply = redisCommand(redis, "PUBLISH %s %s",
                               REDIS_HK_ACK_CHANNEL,
                               ctrl_cmd_param[i].name);
          freeReplyObject(reply);

          message(M_INFO, "AMCP pushed %s->%g.",
                 ctrl_cmd_param[i].name, param_val_curr[i]);

        }
        // TODO: remove me
        //for (i = 0; i < num_ctrl_cmd_param(); i++) {
        //  message(M_INFO, "VAL after : %d %d", i, cmd_change[i]);
        //}

        // Only implement a command if its value changed, or if it was a
        // simple push-button request.
        //if (!ctrl_cmd_is_select(i) && !cmd_change[i] && !amcp_change[i])
        if (!cmd_change[i] && !amcp_change[i])
          continue;

        // OLD logic to detect changes made by the servo; replaced with
        // requirement to explicitly flag amcp_change
        //if (param_val_new[i] != param_val_curr[i] && !cmd_change[i])
        //  amcp_change[i] = 1;

	// TODO: remove me
        //message(M_INFO, "new command %d %d (%d %d %d)", i, ctrl_cmd_sys(i),
        //                ctrl_cmd_is_select(i), cmd_change[i], amcp_change[i]);
        //message(M_INFO, "broken %d %d %d", cmd_change[i], i, num_ctrl_cmd_param());

        // reset the command change flags now that the values are pushed
        cmd_change[i] = 0;
        amcp_change[i] = 0;
      }

      last_frame_pos = get_frame_pos();
      send_amcp_changes = 0;
    } else {
      usleep(1);
    }
  }

  return NULL;
}

void write_to_sync(const char *cmd) {
  #ifndef __NO_SYNC_BOX__
    int i, j, len;

    // Send command to sync box.
    len = strlen(cmd);
    for (i = len; i; i -= j) {
      if ((j = write(fd_sync, cmd + len - i, strlen(cmd))) < 0)
        message(M_FATAL, "Unable to write to %s.", SYNC_DEVICE);
    }
  #else
    message(M_WARN, "sync box not present (__NO_SYNC_BOX__ flag set).  "
                    "Ignoring write_to_sync().");
  #endif
}

void read_from_sync(char *buf) {
  #ifndef __NO_SYNC_BOX__
    char read_buf[1024];
    int i, len;
    time_t t_start;

    t_start = time(NULL);

    for (i = 0, strcpy(buf, "");; i++) {
      if ((len = read(fd_sync, read_buf, 1024)) < 0)
        message(M_FATAL, "Unable to read from %s.", SYNC_DEVICE);
      read_buf[len] = '\0';
      strcat(buf, read_buf);
      if (strlen(buf) > 7) {
        if (!strcmp(buf + strlen(buf) - 7, "Synco> "))
          break;
      }

      usleep(1);
      if (time(NULL) - t_start >= SYNC_RESPONSE_TIMEOUT) {
        message(M_ERROR, "Timeout waiting for response from sync box.");
        break;
      }
    }
  #else
    message(M_WARN, "sync box not present (__NO_SYNC_BOX__ flag set).  "
                    "Ignoring read_from_sync().");
  #endif
}

void command_sync(int index) {
  message(M_ERROR, "Why am I coming here?");
  return;

  #ifndef __NO_SYNC_BOX__
    char buf[1024], *ptr_start, *ptr_end, *buf_end;

    if (!strcmp(ctrl_cmd_param[index].name, "sync_rowlen"))
      sprintf(buf, "rl %d\r\n", (int)param_val_curr[index]);
    else if (!strcmp(ctrl_cmd_param[index].name, "sync_numrows"))
      sprintf(buf, "nr %d\r\n", (int)param_val_curr[index]);
    else if (!strcmp(ctrl_cmd_param[index].name, "sync_framerate"))
      sprintf(buf, "fr %d\r\n", (int)param_val_curr[index]);
    else if (!strcmp(ctrl_cmd_param[index].name, "sync_framenum")) {
      sprintf(buf, "fn %d\r\n", (int)param_val_curr[index]);
      param_val_curr[index] = -1;  // So that we can keep sending the same value.
    } else if (!strcmp(ctrl_cmd_param[index].name, "sync_manchout")) {
      if (param_val_curr[index]) {
        sprintf(buf, "go\r\n");
      } else {
        sprintf(buf, "st\r\n");
      }
    } else {
      message(M_ERROR, "sync command %s unknown!", ctrl_cmd_param[index].name);
      return;
    }

    // Write this command.
    write_to_sync(buf);
    // Read back prompt.
    read_from_sync(buf);

    // Now get the current status and print it out.
    strcpy(buf, "?\r\n");
    write_to_sync(buf);
    read_from_sync(buf);
    message(M_INFO, "sync box current state is:");
    ptr_start = strchr(buf, '\n') + 1;
    buf_end = buf + strlen(buf) + 1;
    for (;;) {
      if ((ptr_end = strchr(ptr_start, '\n')) != NULL)
        *ptr_end = '\0';
      if (!strcmp(ptr_start + strlen(ptr_start) - 7, "Synco> ") ||
          ptr_start >= buf_end)
        break;
      if (!strlen(ptr_start))
        continue;
      message(M_INFO, "  %s", strip_newline(ptr_start));
      ptr_start = ptr_end + 1;
    }
  #else
    message(M_WARN, "sync box not present (__NO_SYNC_BOX__ flag set).  "
                    "Ignoring command_sync().");
  #endif
}

void write_ctrl_cmd_vals(int pos, int page) {
  int i;
  unsigned short *ptr_int, bit, mask;
  float *ptr_flt;

  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    if (!(pos % all_ctrl_cmd_period[i])) {
      //map_ctrl_cmd_to_frame(i, pos, page);
      //if (ctrl_cmd_bit_map[i] >= 0) {
      //  ptr_int = (unsigned short *)map_ctrl_cmd_to_frame(i, pos, page);
      //  bit = (param_val_curr[i] ? 1 : 0) << ctrl_cmd_bit_map[i];
      //  mask = ~(1 << ctrl_cmd_bit_map[i]);
      //  *ptr_int = ((*ptr_int) & mask) | bit;
      //} else {
        ptr_flt = (float *)map_ctrl_cmd_to_frame(i, pos, page);
        *ptr_flt = (float)param_val_curr[i];
      //}
    }
  }
}
