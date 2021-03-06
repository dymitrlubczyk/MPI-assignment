#include "mpi.h"

void master(int node_count, char init_mode);
void worker(int node_count, int id);
void send_result(int stop, int result);
int get_results(MPI_Request *result_requests, int node_count);
int distribute_work(MPI_Request *work_request, int *A, int tasks_count, int next_task, int node_count);
int get_stop(int stop, MPI_Request stop_request);
int* get_task();
void send_task(int node, int task, int *A);
int *initialise(char init_mode);
void finish(MPI_Request *result_requests, MPI_Request *work_requests, int *A, int tasks_count, int next_task, int node_count);