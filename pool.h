#ifndef POOL_H
#define POOL_H
#include <stdio.h>
#include <stdlib.h>

#define true 1
#define false 0

typedef int bool;

typedef struct s_pool_list PoolList;
typedef struct s_pool Pool;

void pool_init(Pool *p, int id, int n);
void pool_destroy(Pool *p);
void pool_push(Pool *p, int value);
int pool_pop(Pool *p);
int* pool_to_vector(Pool *p);
void pool_print(Pool *p);

void pool_list_init(PoolList *pl, int first, int size);
int pool_list_pop(PoolList *pl, int p);
bool pool_list_isEmpty(PoolList *pl);
bool pool_list_poolIsEmpty(PoolList *pl, int p);
void pool_list_fill(PoolList *pl, int n);
void pool_list_print(PoolList *pl);

struct s_pool {
    int id;
    int *vector;
    int size;
    int max_size;
};

struct s_pool_list {
    Pool **pools;
    int size;
    int init_p;
};

#endif
