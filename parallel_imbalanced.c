#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "parallel_imbalanced.h"

const int N = 500;
const int R = 100;
const int WORK_TAG = 1;
const int STOP_TAG = 2;
const int RESULT_TAG = 3;
const int TASK_SIZE = 10;

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
            next_task = distribute_work(work_requests, A, tasks_count, next_task, node_count);
            counter += get_results(result_requests, node_count);
            counter += test_imbalanced(A[TASK_SIZE * my_task + i]);
        }

        my_task = next_task;
        next_task++;
    }

    for (int i = 1; i < node_count; ++i)
        send_stop(i);

    double end = MPI_Wtime();

    printf("Execution time: %fs\n", end - start);
}

void worker(int node_count, int id)
{
    int result, task_ready;
    int stop = 0;
    int *task;

    MPI_Request work_request;
    //MPI_Irecv(task, TASK_SIZE, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, &work_request);

    MPI_Request stop_request;
    MPI_Irecv(&result, 1, MPI_INT, 0, STOP_TAG, MPI_COMM_WORLD, &stop_request);

    while (!stop)
    {
     //   send_ready(stop);
        task = get_task();

    //    while (!stop && !task_ready)
    //    {
    //        stop = get_stop(stop_request);
    //        task_ready = get_task(work_request, task);
    //    }

        for (int i = 0; i < TASK_SIZE && !stop; ++i)
        {
            int result = test_imbalanced(task[i]);
            printf("Number %d tested", task[i]);
            stop = get_stop(stop_request);
            send_result(stop, result);
        }
    }
}

void send_ready(int stop)
{
    if (!stop)
    {
        printf("I am ready\n");
        int ready = 1;
        MPI_Request ready_request;
        MPI_Isend(&ready, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, &ready_request);
    }
}

void send_result(int stop, int result)
{
    if (!stop && result)
        MPI_Send(&result, 1, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
}

int get_results(MPI_Request *result_requests, int node_count)
{
    int result, counter = 0;

    for (int i = 1; i < node_count; ++i)
    {
        int ready = 0;
        MPI_Status status;
        MPI_Test(&result_requests[i], &ready, &status);

        if (ready)
        {
            counter += 1;
            MPI_Irecv(&result, 1, MPI_INT, i, RESULT_TAG, MPI_COMM_WORLD, &result_requests[i]);
        }
    }

    return counter;
}

int distribute_work(MPI_Request *work_requests, int *A, int tasks_count, int next_task, int node_count)
{
    for (int i = 1; i < node_count; ++i)
    {
        int requested = 0;

        MPI_Test(&work_requests[i], &requested, MPI_STATUS_IGNORE);

        if (requested)
            next_task < tasks_count ? send_task(i, next_task++, A, &work_requests[i]) : send_stop(i);
    }

    return next_task;
}

int get_stop(MPI_Request stop_request)
{
    int stop = 0;

    MPI_Test(&stop_request, &stop, MPI_STATUS_IGNORE);
    if(stop) printf("Got stop\n");
    return stop;
}

int* get_task()
{
    int ready = 1;
    MPI_Request ready_request;
    MPI_Isend(&ready, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, &ready_request);

    int *task = allocate_mem(TASK_SIZE);

    MPI_Recv(task, TASK_SIZE, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    printf("Task received\n");


    return task;
}

void send_stop(int node)
{
    int stop = 1;

    MPI_Request stop_request;

    MPI_Isend(&stop, 1, MPI_INT, node, STOP_TAG, MPI_COMM_WORLD, &stop_request);
}

void send_task(int node, int task, int *A, MPI_Request *work_request)
{
    int result;
    MPI_Irecv(&result, 1, MPI_INT, node, WORK_TAG, MPI_COMM_WORLD, work_request);

    printf("Sending task %d to %d\n", task, node);
    MPI_Send(&A[task * TASK_SIZE], TASK_SIZE, MPI_INT, node, WORK_TAG, MPI_COMM_WORLD);
    printf("Done\n");
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