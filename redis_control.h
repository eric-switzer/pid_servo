#ifndef REDIS_CONTROL_H
#define REDIS_CONTROL_H

void redis_control_init();
void *redis_control_thread(void *arg);

#endif
