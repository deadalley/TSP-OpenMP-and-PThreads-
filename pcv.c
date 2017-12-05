#include "pcv.h"

int N, P, T, min = MAX_VAL, min_path_i;

void matrix_print(int **matrix, int size) {
    printf("Printing matrix of size %d...\n", size);
    int i, j;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++)
            printf("\tMatrix[%d][%d]: %d\n", i, j, matrix[i][j]);
    }
    printf("Finished printing matrix...\n");
}

int main (int argc, char **argv) {
    int local_rank, local_psize, tag = 1;
    int err_codes[10];
    MPI_Status status;
    MPI_Comm inter_comm;

    int i, j, r;
    int *data, *minima, *min_paths, recv_buff[2];

    //char _name[20] = "file.in";
    char _hello[] = "Hello slave!";
    char _recvhello[20];

    char *file_name = (char*) malloc (20);

    strcpy(file_name, argv[1]);

    /* Opening file */
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: couldn't open file %s\n", file_name);
        return 1;
    }

    /* Reading file */
    //printf("Reading file %s...\n", file_name);
    size_t buffer_size = 12;
    char *buffer = malloc(buffer_size);

    // Get number of processes
    getline(&buffer, &buffer_size, file);
    P = atoi(buffer);

    // Get number of threads
    getline(&buffer, &buffer_size, file);
    T = atoi(buffer);

    // Get order of matrix
    getline(&buffer, &buffer_size, file);
    N = atoi(buffer);

    //printf("P: %d\nT: %d\nN: %d\n", P, T, N);

    int matrix[N][N];

    // Fill matrix
    i = j = 0;
    while (getline(&buffer, &buffer_size, file) != -1) {
        //printf("\tRead %d from file\n", atoi(buffer));
        matrix[i][j++] = atoi(buffer);
        if (j == N) {
            j = 0;
            i++;
        }
    }
    fflush(stdout);
    free(buffer);
    fclose(file);
    //matrix_print(matrix, N);

    /* Starting MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &local_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &local_psize);

    // Spawning slaves
    //printf("Spawning %d slaves...\n", P);
    MPI_Comm_spawn("slave", MPI_ARGV_NULL, P, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &inter_comm, err_codes);

    /* Waiting for all slaves to spawn */
    //printf("Sending message to slaves...\n");
    for (i = 0; i < P; i++) {
        MPI_Send(_hello, 20, MPI_CHAR, i, tag, inter_comm);
        MPI_Recv(_recvhello, 20, MPI_CHAR, i, tag, inter_comm, &status);
        //printf("Slave %d says: %s\n", i, _recvhello);
    }

    /* Sending N and T */
    data = (int*) malloc(sizeof(int)*2);
    data[0] = N;
    data[1] = T;
    MPI_Send(data, 2, MPI_INT, 0, tag, inter_comm);
    free(data);

    /* Sending start and R */
    data = (int*) malloc(sizeof(int)*P*2);
    j = 1;
    r = N/P;
    if (r == 0)
        r = 1;

    // Calculating start and r for each process
    for (i = 0; i < 2*P; i += 2) {
        data[i] = j;
        j += r;

        if (i+2 >= 2*P && (N-1)%r != 0)
            r = (N-1)%r;
        data[i+1] = r;
    }
    /*
    printf("Printing data...\n");
    for (i = 0; i < 2*P; i += 2)
        printf("P%d: start: %d, R: %d\n", i/2, data[i], data[i+1]);
    */
    MPI_Scatter(data, 2, MPI_INT, recv_buff, 2, MPI_INT, MPI_ROOT, inter_comm);
    free(data);

    /* Sending matrix */
    data = (int*) malloc(sizeof(int)*N*N);
    for (i = 0; i < N*N; i++) {
        data[i] = matrix[i/N][i%N];
        //printf("\tdata[%d]: %d\n", i, data[i]);
    }

    MPI_Send(data, N*N, MPI_INT, 0, tag, inter_comm);

    /* Gathering results */
    minima = (int*) malloc(sizeof(int)*P);
    MPI_Gather(recv_buff, 2, MPI_INT, minima, P, MPI_INT, MPI_ROOT, inter_comm);

    min_paths = (int*) malloc(sizeof(int)*P*N);
    MPI_Gather(recv_buff, 2, MPI_INT, min_paths, N, MPI_INT, MPI_ROOT, inter_comm);

    for (i= 0; i < P; i++) {
        if (minima[i] == MAX_VAL)
            continue;

        if (minima[i] < min) {
            min = minima[i];
            min_path_i = i;
        }
        /*
        printf("\tminima[%d]: %d, min_paths[%d]: ", i, minima[i], i);
        for (j = 0; j < N; j++)
            printf(" %d,", min_paths[(i*N)+j]);
        printf("\n");
        */
    }

    /* Final communication */
    MPI_Recv(_recvhello, 20, MPI_CHAR, 0, tag, inter_comm, &status);
    MPI_Send(_hello, 20, MPI_CHAR, 0, tag, inter_comm);
    //printf("Master out...\n");

    MPI_Finalize();

    /* Output */
    printf("Caminho minimo: 0 ");
    for (i = 0; i < N; i++)
        printf("%d ", min_paths[(min_path_i*N)+i]);

    printf("\nCusto minimo: %d\n", min);

    return 0;
}
