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
#include "servo.h"
#include "control.h"

int max_ctrl_cmd_rate;
double *param_val_curr;
// flag: external command received or amcp request
int *cmd_change = NULL;
int *amcp_change = NULL;

void message_handler(redisAsyncContext *c, void *reply, void *privdata);

int num_ctrl_cmd_param() {
  static int num = -1;

  if (num < 0)
    for (num = 0; strlen(ctrl_cmd_param[num].name); num++);

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

void control_init() {
  redisContext *redis;
  redisReply *reply;
  int i, j, n, index, bit;
  int push_default = 1;

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
  }
  message(M_START, "Finished retrieving current command values.");

  // Start the subscriber thread here? or in amcp.c
  //pthread_create(&command_loop, NULL, command_thread, NULL);
}

void message_handler(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;
  int i, k, cmdindex;
  double cmdval;
  char *sp;

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
        if (!cmd_change[i] && !amcp_change[i])
          continue;

        // OLD logic to detect changes made by the servo; replaced with
        // requirement to explicitly flag amcp_change
        //if (param_val_new[i] != param_val_curr[i] && !cmd_change[i])
        //  amcp_change[i] = 1;

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
