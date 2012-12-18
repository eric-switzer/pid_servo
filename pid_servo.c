#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "read_temp.h"
#include "servo_temp.h"

int main(int argc, char *argv[]) {
  pthread_t read_loop, servo_loop;

  servo_temp_init();
  read_temp_init();

  pthread_create(&read_loop, NULL, read_temp_thread, NULL);
  pthread_create(&servo_loop, NULL, servo_temp_thread, NULL);

  for (;;)
    usleep(1);

  return 0;
}
