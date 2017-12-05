#ifndef SLAVE_H
#define SLAVE_H
#include <pthread.h>
#include "pool.h"
#include "pcv.h"

Pool* min_path(int p, int *vertices, int size, int *ret_min);
void *thread_start(void *arg);
void *thread_debug(void *arg);
void _debugMinPath();

#endif
