#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "parallel_balanced.h"

int main(int argc, char *argv[])
{
    const int N = 500;
    const int R = 100;

    int id, node_count;
    char init_mode = argv[1][0];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &node_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    id == 0 ? master(N, R, node_count) : worker(N, R, node_count, id);

    MPI_Finalize();

    return 0;
}

void master(int N, int R, int node_count, char init_mode)
{
    double start, end;
    int counter = 0;

    int *A = initialise(N, init_mode);
    int task_size = send_tasks(A, N, node_count);
    MPI_Request *result_requests = initialise_requests(node_count);

    start = MPI_Wtime();

    for (int i = 0; i < task_size && counter < R; ++i)
    {
        counter += test(A[i]);
        counter += get_results(result_requests, node_count);
    }

    send_stop(node_count);

    end = MPI_Wtime();

    printf("Execution time: %fs\n", end - start);
}

void worker(int N, int R, int node_count, int id)
{
    int result;
    MPI_Request stop_request;
    MPI_Irecv(&result, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stop_request);

    int task_size = N / node_count;
    int *task = get_task(task_size);

    for (int i = 0; i < task_size && !get_stop(stop_request); ++i)
        send_result(test(task[i]));
}

int get_stop(MPI_Request stop_request)
{
    int stop = 0;

    MPI_Test(&stop_request, &stop, MPI_STATUS_IGNORE);

    return stop;
}

void send_stop(int node_count)
{
    for (int i = 1; i < node_count; ++i)
    {
        int stop = 1;

        MPI_Request stop_request;

        MPI_Isend(&stop, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &stop_request);
    }
}

int get_results(MPI_Request *result_requests, int node_count)
{
    int result, counter = 0;

    for (int i = 1; i < node_count; ++i)
    {
        int ready = 0;
        MPI_Test(&result_requests[i], &ready, MPI_STATUS_IGNORE);

        if (ready)
        {
            counter += 1;
            MPI_Irecv(&result, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &result_requests[i]);
        }
    }

    return counter;
}

void send_result(int result)
{
    if (result)
        MPI_Send(&result, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
}

int *get_task(int task_size)
{
    int *task = allocate_mem(task_size);

    MPI_Recv(task, task_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return task;
}

int send_tasks(int *A, int N, int node_count)
{
    int task_size = N / node_count;
    int master_task_size = N - (node_count - 1) * task_size;

    for (int i = 1; i < node_count; ++i)
    {
        MPI_Request task_request;
        MPI_Isend(&A[master_task_size + (i - 1) * task_size], task_size, MPI_INT, i, 0, MPI_COMM_WORLD, task_request);
    }

    return master_task_size;
}

MPI_Request *initialise_requests(int node_count)
{
    MPI_Request *requests = calloc(node_count, sizeof(MPI_Request));
    int result;

    for (int i = 1; i < node_count; ++i)
        MPI_Irecv(&result, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &requests[i]);

    return requests;
}

int *initialise(int N, char init_mode)
{
    int *A = allocate_mem(N);
    init_mode == 'r' ? fill_random(A, N) : fill_ascending(A, N);
    return A;
}