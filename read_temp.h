#ifndef READ_TEMP_H
#define READ_TEMP_H

#define READTHREAD_OUTPUT "readthread.dat"
void read_temp_init();
void *read_temp_thread(void *arg);

#define READ_RATE 10000

#endif
