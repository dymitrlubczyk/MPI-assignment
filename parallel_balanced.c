#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "parallel_balanced.h"

const int N = 500;
const int R = 100;

const int RESULT_TAG = 1;
const int STOP_TAG = 2;
const int WORK_TAG = 3;

int main(int argc, char *argv[])
{

    int id, node_count;
    char init_mode = argv[1][0];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &node_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    id == 0 ? master(node_count, init_mode) : worker(node_count, id);

    printf("Node %d is done\n", id);
    MPI_Finalize();

    return 0;
}

void master(int node_count, char init_mode)
{
    double start, end;
    int counter = 0;

    int *A = initialise(init_mode);
    int task_size = send_tasks(A, node_count);

    int result;
    MPI_Request result_request;
    MPI_Irecv(&result, 1, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &result_request);

    start = MPI_Wtime();

    for (int i = 0; i < task_size && counter < R; ++i)
    {
        counter += test(A[i]);
        counter += get_results(&result_request, node_count);
        printf("Counter: %d\n", counter);
    }

    send_stop(&result_request, node_count);
    free(A);

    end = MPI_Wtime();
    printf("Execution time: %fs\n", end - start);
}

void worker(int node_count, int id)
{
    int stop = 0;
    int task_size = N / node_count;
    int *task = get_task(task_size);

    int stop_result;
    MPI_Request stop_request;
    MPI_Irecv(&stop_result, 1, MPI_INT, 0, STOP_TAG, MPI_COMM_WORLD, &stop_request);

    for (int i = 0; i < task_size && !stop; ++i)
    {
        int result = test(task[i]);
        stop = get_stop(stop_request);
        send_result(stop, result);
    }

    free(task);
}

int get_stop(MPI_Request stop_request)
{
    int stop = 0;
    MPI_Test(&stop_request, &stop, MPI_STATUS_IGNORE);
    return stop;
}

void send_stop(MPI_Request *result_request, int node_count)
{

    for (int i = 1; i < node_count; ++i)
    {
        int stop = 1;
        MPI_Request stop_request;
        MPI_Isend(&stop, 1, MPI_INT, i, STOP_TAG, MPI_COMM_WORLD, &stop_request);
    }

    get_results(result_request, node_count);
}

int get_results(MPI_Request *result_request, int node_count)
{
    int ready = 1;
    int counter = 0;

    while (ready)
    {
        MPI_Status status;
        MPI_Test(result_request, &ready, &status);

        if (ready && status.MPI_SOURCE > 0 && status.MPI_SOURCE < node_count)
        {
            counter++;

            int result;
            MPI_Request new_request;
            MPI_Irecv(&result, 1, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &new_request);

            *result_request = new_request;
        }

        else
            ready = 0;
    }

    return counter;
}

void send_result(int stop, int result)
{
    if (!stop && result)
        MPI_Send(&result, 1, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
}

int *get_task(int task_size)
{
    int *task = allocate_mem(task_size);
    MPI_Recv(task, task_size, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return task;
}

int send_tasks(int *A, int node_count)
{
    int task_size = N / node_count;
    int master_task_size = N - (node_count - 1) * task_size;

    for (int i = 1; i < node_count; ++i)
        MPI_Send(&A[master_task_size + (i - 1) * task_size], task_size, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD);

    return master_task_size;
}

int *initialise(char init_mode)
{
    int *A = allocate_mem(N);
    init_mode == 'r' ? fill_random(A, N) : fill_ascending(A, N);
    return A;
}