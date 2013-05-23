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

#include "circular_buffer.h"
#include "control_struct.h"
#include "servo.h"
#include "control.h"
#include "extern.h"

int max_ctrl_cmd_rate;
double *param_val_curr;
// flag: external command received or servo request
int *cmd_change = NULL;
int *servo_change = NULL;

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
  int i;
  int push_default = 1;

  param_val_curr = (double *)safe_malloc("param_val_curr",
                                       num_ctrl_cmd_param() * sizeof(double));

  if (cmd_change == NULL) {
    cmd_change = (int *)safe_calloc("cmd_change", num_ctrl_cmd_param(),
                                   sizeof(int));
  }

  if (servo_change == NULL) {
    servo_change = (int *)safe_calloc("servo_change", num_ctrl_cmd_param(),
                                    sizeof(int));
  }

  // Initialize the parameter values to their defaults
  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    param_val_curr[i] = ctrl_cmd_param[i].default_val;
  }

  redis = redisConnect(REDIS_HOST, REDIS_HOST_PORT);
  if (redis->err) {
    fprintf(stderr, "Connection error: %s\n", redis->errstr);
    exit(EXIT_FAILURE);
  }

  if (push_default) {
    printf("Writing default command values to the server.\n");
  } else {
    printf("Getting current command values.\n");
  }

  for (i = 0; i < num_ctrl_cmd_param(); i++) {
    if (push_default) {
      reply = redisCommand(redis, "SET %s %10.15g",
                           ctrl_cmd_param[i].name,
                           ctrl_cmd_param[i].default_val);

      printf("REDIS: set %s %10.15g returns %s\n",
             ctrl_cmd_param[i].name,
             ctrl_cmd_param[i].default_val,
             reply->str);

      freeReplyObject(reply);
    } else {
      reply = redisCommand(redis, "GET %s", ctrl_cmd_param[i].name);

      if ( reply->type == REDIS_REPLY_ERROR ) {
        printf("Error: %s\n", reply->str);
      } else if ( reply->type != REDIS_REPLY_STRING ) {
        printf("Unexpected type/no value: %d\n", reply->type);
      } else {
        sscanf(reply->str, "%lf", &param_val_curr[i]);
        printf("Result for %s: %10.15g\n",
               ctrl_cmd_param[i].name,
               param_val_curr[i]);
      }
      freeReplyObject(reply);
    }
  }
  printf("Finished retrieving current command values.\n");

  return;
}


void message_handler(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;
  int cmdindex;
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
            printf("Received command %s (idx=%d) with value %g.\n",
                    ctrl_cmd_param[cmdindex].name,
                    cmdindex,
                    param_val_curr[cmdindex]);
          } else {
            printf("Unknown system variable received: %s\n",
                    r->element[2]->str);
          }
        } else {
          printf("Malformed command received: %s\n", r->element[2]->str);
        }
      }
    }
    if (!strcmp(r->element[1]->str, "messages")) {
      printf("received a log message: handle this?\n");
    }
  }

  return;
}

void *command_thread(void *arg) {
  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  printf("starting sub conn\n");
  redisAsyncContext *c = redisAsyncConnect(REDIS_HOST, REDIS_HOST_PORT);
  if (c->err) {
    printf("REDIS not connected: %s\n", c->errstr);
  }

  redisLibeventAttach(c, base);
  redisAsyncCommand(c, message_handler, NULL, "SUBSCRIBE housekeeping");
  event_base_dispatch(base);
  return 0;
}

void *control_thread(void *arg) {
  int i;

  redisContext *redis;
  redisReply *reply;

  redis = redisConnect(REDIS_HOST, REDIS_HOST_PORT);
  if (redis->err) {
    printf("Redis connection error: %s\n", redis->errstr);
    exit(EXIT_FAILURE);
  }

  for (;;) {
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

        printf("Ack command: %s->%g.\n",
               ctrl_cmd_param[i].name, param_val_curr[i]);

        // reset and also block a change by servo if made at the same time
        if (servo_change[i]) {
          servo_change[i] = 0;
        }
      } else if (servo_change[i]) {
        reply = redisCommand(redis, "SET %s %10.15g",
                             ctrl_cmd_param[i].name,
                             param_val_curr[i]);
        freeReplyObject(reply);

        reply = redisCommand(redis, "PUBLISH %s %s",
                             REDIS_HK_ACK_CHANNEL,
                             ctrl_cmd_param[i].name);
        freeReplyObject(reply);

        printf("servo pushed %s->%g.\n",
               ctrl_cmd_param[i].name, param_val_curr[i]);

      }

      // Only implement a command if its value changed, or if it was a
      // simple push-button request.
      if (!cmd_change[i] && !servo_change[i])
          continue;

      // reset the command change flags now that the values are pushed
      cmd_change[i] = 0;
      servo_change[i] = 0;
    }
  }

  return NULL;
}
