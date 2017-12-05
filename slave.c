#include "slave.h"

int N, T, R;
int start, p, min, p_min;
int *minima;
int **matrix;
Pool **paths;

pthread_mutex_t p_lock, min_lock, pl_lock;
PoolList *pool_list;

void *thread_debug(void *arg) {
    int id = (int) *((int *)arg);
    printf("Debugging Thread %d...\n", id);
    return NULL;
}

void *thread_start(void *arg) {
    int id = (int) *((int *)arg);
    //printf("Thread %d rolling...\n", id);

    int local_p, local_sub, local_min;
    local_p = local_sub = -1;

    bool pl_isEmpty;

    // Check if pool list is empty
    pthread_mutex_lock(&pl_lock);
    pl_isEmpty = pool_list_isEmpty(pool_list);
    pthread_mutex_unlock(&pl_lock);

    // If start > N-1, there is no min_path to find here
    if (start > N-1)
        return NULL;

    while (!pl_isEmpty) {
        Pool *local_path;

        /* Get p for finding min path - CRITICAL REGION*/
        pthread_mutex_lock(&p_lock);
        while(p < start+R && pool_list_poolIsEmpty(pool_list, p))
            p++;

        // All min path have been calculated
        if (p >= start+R) {
            pthread_mutex_unlock(&p_lock);
            return NULL;
        }

        local_p = p;
        local_sub = pool_list_pop(pool_list, local_p);
        //printf("Thread %d has local_p = %d e local_sub = %d...\n", id, local_p, local_sub);
        pthread_mutex_unlock(&p_lock);

        /* Find min */
        // Generate vertices for min path function
        int *vertices = (int*) malloc(sizeof(int)*(N-3));
        int k = 1;
        int i;
        for (i = 0; i < N-3 ; i++) {
            while (k == local_p || k == local_sub)
                k++;

            vertices[i] = k;
            //printf("\tT%d: Vertices[%d]: %d\n", id, i, vertices[i]);
            k++;
        }

        // Find min path and local min
        local_path = min_path(local_sub, vertices, N-3, &local_min);
        local_min = matrix[local_p][local_sub] + local_min;
        //printf("\tFound local_min: %d\n", local_min);

        /* Update min and min path - CRITICAL REGION */
        pthread_mutex_lock(&min_lock);
        if (local_min < minima[local_p-start]) {
            pool_push(local_path, local_p);
            paths[local_p-start] = local_path;
            minima[local_p-start] = local_min;
        }

        else pool_destroy(local_path);
        pthread_mutex_unlock(&min_lock);

        /* Check if all min paths have been found - CRITICAL REGION */
        pthread_mutex_lock(&pl_lock);
        pl_isEmpty = pool_list_isEmpty(pool_list);
        pthread_mutex_unlock(&pl_lock);

    }

    return NULL;
}

Pool* min_path(int p, int *vertices, int size, int *ret_min) {
    int i, j, min = MAX_VAL;
    /*
    printf("\tMIN_PATH: p = %d vertices: {", p);
    for (i = 0; i < size; i++)
        printf("%d ", vertices[i]);
    printf("}\n");
    */
    if (size == 1) {
        Pool *p_path = (Pool*) malloc(sizeof(Pool));
        pool_init(p_path, p, N);

        pool_push(p_path, 0);
        pool_push(p_path, vertices[0]);
        pool_push(p_path, p);
        *ret_min = matrix[p][vertices[0]] + matrix[vertices[0]][0];
        return p_path;
    }

    Pool *p_path;
    // Calculate every min path for vertices recursively
    for (i = 0; i < size; i++) {
        int p1;
        p1 = vertices[i];

        int *v1;
        v1 = (int*) malloc(sizeof(int)*(size-1));

        // Generate new vertices
        int k = 0;
        for (j = 0; j < size-1; j++) {
            if (vertices[k] == p1) {
                k++;
                if (j >= size)
                    break;
            }
            v1[j] = vertices[k];
            k++;
        }

        int new_min;
        Pool *new_path;
        new_path = min_path(p1, v1, size-1, &new_min);
        new_min = matrix[p][vertices[i]] + new_min;
        //printf("\t\tFound new_min: %d\n", new_min);

        if (new_min < min) {
            pool_push(new_path, p);
            p_path = new_path;
            min = new_min;
            //pool_print(p_path);
        }
        //else (pool_destroy(p_path));
        free(v1);
    }

    *ret_min = min;
    return p_path;
}

void _debugMinPath() {
    int local_p = 0;
    int local_sub = 3;

    int *vertices = (int*) malloc(sizeof(int)*(N-1));
    int k = 0, i;
    for (i = 0; i < N-1 ; i++) {
        if (k == local_p)
            k++;
        //if (k == local_sub)
            //k++;

        vertices[i] = k;
        printf("\tVertices[%d]: %d\n", i, vertices[i]);
        k++;
    }

    //Pool *local_path = (Pool*) malloc(sizeof(Pool));
    Pool *local_path;
    int local_min;
    local_path = min_path(local_p, vertices, N-1, &local_min);
    printf("\tFound min: %d\n", local_min);
    //pool_print(local_path);
}

int main(int argc, char **argv) {
    int local_rank, local_psize, tag = 1;
    MPI_Status status;
    MPI_Comm inter_comm;

    int i;
    int *data, recv_buff[2];

    char _hello[] = "Hello master!";
    char _recvhello[20];

    /* Starting MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &local_psize);
    MPI_Comm_get_parent(&inter_comm);

    //printf("Slave(%d) started...\n", local_rank);

    /* Hello */
    MPI_Recv(_recvhello, 20, MPI_CHAR, 0, tag, inter_comm, &status);
    //printf("Slave (%d), master says: %s\n", local_rank, _recvhello);
    MPI_Send(_hello, 20, MPI_CHAR, 0, tag, inter_comm);

    data = (int*) malloc(sizeof(int)*2);

    /* Communication with master */
    if (local_rank == 0) {
        // Getting N and T
        MPI_Recv(data, 20, MPI_INT, 0, tag, inter_comm, &status);
    }

    // Broadcasting N and T
    MPI_Bcast(data, 2, MPI_INT, 0, MPI_COMM_WORLD);
    N = data[0];
    T = data[1];
    //printf("Slave (%d) received: N: %d, T: %d\n", local_rank, N, T);

    // Receiving start and R
    MPI_Scatter(recv_buff, 2, MPI_INT, data, 2, MPI_INT, 0, inter_comm);
    start = data[0];
    R = data[1];
    //printf("\tSlave(%d) received: start: %d, R: %d\n", local_rank, start, R);
    free(data);

    /* Communication with master */
    data = (int*) malloc(sizeof(int)*N*N);
    if (local_rank == 0) {
        // Getting matrix
        MPI_Recv(data, N*N, MPI_INT, 0, tag, inter_comm, &status);
    }

    // Broadcasting matrix
    MPI_Bcast(data, N*N, MPI_INT, 0, MPI_COMM_WORLD);

    // Setting up matrix
    matrix = (int**) malloc(sizeof(int*)*N);
    for (i = 0; i < N; i++) {
        matrix[i] = (int*) malloc(sizeof(int)*N);
    }

    for (i = 0; i < N*N; i++) {
        matrix[i/N][i%N] = data[i];
    }
    free(data);

    /* Initializing minima */
    minima = (int*) malloc(sizeof(int)*(R));
    paths = (Pool**) malloc(sizeof(Pool*)*(R));

    for (i = 0; i < R; i++) {
        minima[i] = MAX_VAL;
        paths[i] = NULL;
    }

    /* Initializing pool list */
    pool_list = (PoolList*) malloc(sizeof(PoolList));
    pool_list_init(pool_list, start, R);
    pool_list_fill(pool_list, N);
    //pool_list_print(pool_list);

    // Initializing p and min
    p = start;
    min = MAX_VAL;

    /* Initializing mutex */
    if (pthread_mutex_init(&p_lock, NULL) != 0) {
        fprintf(stderr, "Error: p_lock mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&min_lock, NULL) != 0) {
        fprintf(stderr, "Error: min_lock mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&pl_lock, NULL) != 0) {
        fprintf(stderr, "Error: pl_lock mutex init failed\n");
        return 1;
    }

    /* Initializing threads */
    pthread_t threads[T];
    int id[T];
    //printf("Slave(%d) Creating %d threads...\n", local_rank, T);
    for (i = 0; i < T; i++) {
        id[i] = i;
        if (pthread_create(&threads[i], NULL, &thread_start, &id[i]) != 0) {
            fprintf(stderr, "Error: couldn't create thread %d\n", id[i]);
        }
    }

    /* Joining threads */
    //printf("Slave(%d) Joining threads...\n", local_rank);
    for (i = 0; i < T; i++) {
        pthread_join(threads[i], 0);
        //printf("Slave(%d) Joined thread %d...\n", local_rank, i);
    }

    /* Waiting for processes to finish */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Finding local min */
    //printf("Slave(%d) Printing minima...\n", local_rank);
    for (i = 0; i < R; i++) {
        if (minima[i] + matrix[0][start + i] < min) {
            min = minima[i] + matrix[0][start + i];
            p_min = i;
        }
        /*
        printf("\t%d: %d\n", i+start, minima[i]);
        if (paths[i] != NULL)
            pool_print(paths[i]);
        */
    }

    /* Sending local min with path to master */
    //printf("Slave(%d) Found min at %d...\n", local_rank, min);
    MPI_Gather(&min, 1, MPI_INT, recv_buff, 2, MPI_INT, 0, inter_comm);

    // If min path was found, convert to vector and send
    if (paths[p_min] != NULL)
        data = pool_to_vector(paths[p_min]);
    // Send trash otherwise
    else {
        data = (int*) malloc(sizeof(int)*N);
        for (i = 0; i < N; i++)
            data[i] = -1;
    }
    MPI_Gather(data, N, MPI_INT, recv_buff, 2, MPI_INT, 0, inter_comm);

    /* Communication with master */
    if (local_rank == 0) {
        MPI_Send(_hello, 20, MPI_CHAR, 0, tag, inter_comm);
        MPI_Recv(_recvhello, 20, MPI_CHAR, 0, tag, inter_comm, &status);
    }
    //printf("Slave(%d) out...\n", local_rank);
    MPI_Finalize();

    return 0;
}
