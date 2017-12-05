#include "pool.h"

void pool_list_init(PoolList *pl, int first, int size) {
    pl->size = size;
    pl->init_p = first;

    pl->pools = (Pool**) malloc(sizeof(Pool*)*size);
}

void pool_list_fill(PoolList *pl, int n) {
    int i;
    for (i = 0; i < pl->size; i++) {
        Pool *p = (Pool*) malloc(sizeof(Pool));
        pool_init(p, pl->init_p+i, n-2);

        int j;
        for (j = n-1; j > 0; j--) {
            if (j == (pl->init_p + i))
                j--;
            if (j <= 0)
                continue;
            pool_push(p, j);
        }
        //pool_print(p);

        pl->pools[i] = p;
    }
}

int pool_list_pop(PoolList *pl, int p) {
    int k = p - pl->init_p;
    return pool_pop(pl->pools[k]);
}

void pool_list_print(PoolList *pl) {
    int i;
    printf("Printing pool list for first %d...\n", pl->init_p);
    for (i = 0; i < pl->size; i++) {
        pool_print(pl->pools[i]);
    }
    printf("Finished printing pool list...\n");
}

bool pool_list_isEmpty(PoolList *pl) {
    int i;
    for (i = 0; i < pl->size; i++) {
      if (pl->pools[i]->size != 0)
          return false;
    }

    return true;
}

bool pool_list_poolIsEmpty(PoolList *pl, int p) {
    if (p < pl->init_p || p > pl->pools[pl->size-1]->id) {
        fprintf(stderr, "Error: %d is not in pool list\n", p);
        return false;
    }

    int k = p - pl->init_p;
    if (pl->pools[k]->size == 0)
        return true;

    return false;
}

void pool_init(Pool *p, int id, int n) {
    p->id = id;
    p->size = 0;
    p->max_size = n;
    p->vector = (int *) malloc (sizeof(int)*n);

    int i;
    for (i = 0; i < n; i++)
        p->vector[i] = -1;
}

void pool_destroy(Pool *p) {
    free(p->vector);
    free(p);
}

void pool_push(Pool *p, int value) {
    if (p->size < p->max_size)
        p->vector[p->size++] = value;

    //else fprintf(stderr, "Error: pool full (pushed %d)\n", value);
}

int pool_pop(Pool *p) {
    if (p->size == 0)
        fprintf(stderr, "Error: pool empty\n");

    else {
        int aux = p->vector[p->size-1];
        p->vector[p->size-1] = -1;
        p->size--;
        return aux;
    }

    return -2;
}

int* pool_to_vector(Pool *p) {
    int *v = (int*) malloc(sizeof(int)*p->size);
    int i = 0;
    while (p->size > 0) {
        v[i] = pool_pop(p);
        i++;
    }
    return v;
}

void pool_print(Pool *p) {
    printf("\tPrinting %d pool...\n", p->id);
    int i;
    for (i = 0; i < p->max_size; i++) {
        printf("\t%d: %d\n", i, p->vector[i]);
    }
}
/*
int main() {
    PoolList *pl = (PoolList*) malloc(sizeof(PoolList));
    int n = 6;
    int r = 2;
    int start = 1;
    pool_list_init(pl, start, r);
    pool_list_fill(pl, n);
    pool_list_pop(pl, 1);
    pool_list_pop(pl, 1);
    pool_list_pop(pl, 1);
    pool_list_pop(pl, 1);
    pool_list_pop(pl, 2);
    pool_list_print(pl);
    //Pool *p = (Pool*) malloc(sizeof(Pool));
    //pool_print(p);
    return 0;
}
*/
