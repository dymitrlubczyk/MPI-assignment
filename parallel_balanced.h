#include "mpi.h"

void master(int N, int R, int node_count);
void worker(int N, int R, int node_count, int id);
int get_stop(MPI_Request request);
void send_stop(int node_count);
int get_results(MPI_Request* requests, int node_count);
void send_result(int result);
int *get_task(int task_size);
int send_tasks(int *A, int N, int node_count);
MPI_Request* initialise_requests(int node_count);
int *initialise(int N, char init_mode);