#include "mpi.h"

void master(int node_count, char init_mode);
void worker(int node_count);
void send_result(int result);
int get_results(MPI_Request *result_requests, int node_count);
int distribute_work(MPI_Request *work_requests, int *A, int tasks_count, int next_task, int node_count);
int get_stop(MPI_Request *stop_request);
int get_task(MPI_Request *work_request, int *task);
void send_stop(int node);
void send_task(int node, int task, int *A, MPI_Request *work_request);
MPI_Request *initialise_requests(int node_count, int tag);
int *initialise(char init_mode);