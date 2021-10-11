#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "parallel_imbalanced.h"

const N = 500;
const R = 100;
const WORK_TAG = 1;
const STOP_TAG = 2;
const RESULT_TAG = 3;
const TASK_SIZE = 10;

int main(int argc, char *argv[])
{
    int id, node_count;
    char init_mode = argv[1][0];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &node_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    id == 0 ? master(node_count, init_mode) : worker(node_count);

    MPI_Finalize();

    return 0;
}

void master(int node_count, char init_mode)
{
    int counter = 0;

    int my_task = 0;
    int next_task = 1;
    int tasks_count = N / TASK_SIZE;

    int *A = initialise(init_mode);
    MPI_Request *result_requests = initialise_requests(node_count, RESULT_TAG);
    MPI_Request *work_requests = initialise_requests(node_count, WORK_TAG);

    double start = MPI_Wtime();

    while (counter < R && my_task < tasks_count)
    {
        for (int i = 0; i < TASK_SIZE && counter < R; ++i)
        {
            next_task = distribute_work(A, tasks_count, next_task, node_count);
            counter += test_imbalanced(A[TASK_SIZE * my_task + i]);
            counter += get_results(result_requests, node_count);
        }

        my_task = next_task;
        next_task++;
    }

    for (int i = 1; i < node_count; ++i)
        send_stop(i);

    double end = MPI_Wtime();

    printf("Execution time: %fs\n", end - start);
}

void worker(int node_count)
{
    int task_ready, result;
    int stop = 0;
    int *task;
    
    MPI_Request *work_request;
    MPI_Irecv(task, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, work_request);

    MPI_Request *stop_request;
    MPI_Irecv(&result, 1, MPI_INT, 0, STOP_TAG, MPI_COMM_WORLD, stop_request);

    while (!stop)
    {
        stop = get_stop(stop_request);
        task_ready = get_task(work_request, task);

        for (int i = 0; i < TASK_SIZE && !stop && task_ready; ++i)
        {
            stop = get_stop(stop_request);
            send_result(test_imbalanced(task[i]));
        }
    }
}

void send_result(int result)
{
    if (result)
        MPI_Send(&result, 1, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
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
            MPI_Irecv(&result, 1, MPI_INT, i, RESULT_TAG, MPI_COMM_WORLD, &result_requests[i]);
        }
    }

    return counter;
}

int distribute_work(MPI_Request work_requests,int *A, int tasks_count, int next_task, int node_count)
{
    for (int i = 1; i < node_count; ++i)
    {
        int finished = 0;
        MPI_Test(&work_requests[i], &finished, MPI_STATUS_IGNORE);

        if (finished)
        {
            next_task += 1;
            next_task < tasks_count ? send_task(i, next_task - 1, A, &work_requests[i]) : send_stop(i);
        }
    }

    return next_task;
}

int get_stop(MPI_Request *stop_request)
{
    int stop = 0;

    MPI_Test(stop_request, &stop, MPI_STATUS_IGNORE);

    return stop;
}

int get_task(MPI_Request *work_request, int *task)
{
    int ready = 0;
    MPI_Test(work_request, &ready, MPI_STATUS_IGNORE);
    if (ready)
    {
        MPI_Irecv(task, TASK_SIZE, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, work_request);
        return 1;
    }

    return 0;
}

void send_stop(int node)
{
    int stop = 1;

    MPI_Request stop_request;

    MPI_Isend(&stop, 1, MPI_INT, node, STOP_TAG, MPI_COMM_WORLD, &stop_request);
}

void send_task(int node, int task, int *A, MPI_Request *work_request)
{
    MPI_Request task_request;
    MPI_Isend(&A[task * TASK_SIZE], TASK_SIZE, MPI_INT, node, WORK_TAG, MPI_COMM_WORLD, &task_request);

    int result;
    MPI_Irecv(&result, 1, MPI_INT, node, WORK_TAG, MPI_COMM_WORLD, work_request);
}

MPI_Request *initialise_requests(int node_count, int tag)
{
    MPI_Request *requests = calloc(node_count, sizeof(MPI_Request));
    int result;

    for (int i = 1; i < node_count; ++i)
        MPI_Irecv(&result, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[i]);

    return requests;
}

int *initialise(char init_mode)
{
    int *A = allocate_mem(N);
    init_mode == 'r' ? fill_random(A, N) : fill_ascending(A, N);
    return A;
}