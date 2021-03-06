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

    int result;
    MPI_Request result_request;
    MPI_Irecv(&result, 1, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &result_request);

    int work_result;
    MPI_Request work_request;
    MPI_Irecv(&work_result, 1, MPI_INT, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &work_request);

    double start = MPI_Wtime();

    while (counter < R && my_task < tasks_count)
    {
        for (int i = 0; i < TASK_SIZE && counter < R; ++i)
        {
            next_task = distribute_work(&work_request, A, tasks_count, next_task, node_count);
            counter += get_results(&result_request, node_count);
            counter += test_imbalanced(A[TASK_SIZE * my_task + i]);
        }

        printf("Counter: %d, done %d/%d\n", counter, next_task, tasks_count);
        my_task = next_task++;
    }

    finish(&result_request, &work_request, A, tasks_count, next_task, node_count);
    
    double end = MPI_Wtime();
    printf("Execution time: %fs\n", end - start);
}

void worker(int node_count, int id)
{
    int stop = 0;
    int *task;

    MPI_Request stop_request;
    int stop_result;
    MPI_Irecv(&stop_result, 1, MPI_INT, 0, STOP_TAG, MPI_COMM_WORLD, &stop_request);

    while (!stop)
    {
        task = get_task();

        for (int i = 0; i < TASK_SIZE && !stop; ++i)
        {
            int result = test_imbalanced(task[i]);
            stop = get_stop(stop, stop_request);
            send_result(stop, result);
        }

        free(task);
    }
}

void send_result(int stop, int result)
{
    if (!stop && result)
        MPI_Send(&result, 1, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
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

int distribute_work(MPI_Request *work_request, int *A, int tasks_count, int next_task, int node_count)
{
    int requested = 1;

    while (requested)
    {
        requested = 0;
        MPI_Status status;
        MPI_Test(work_request, &requested, &status);

        if (requested && status.MPI_SOURCE > 0 && status.MPI_SOURCE < node_count)
        {
            int task = next_task < tasks_count ? next_task++ : 0;
            send_task(status.MPI_SOURCE, task, A);

            int result;
            MPI_Request new_request;
            MPI_Irecv(&result, 1, MPI_INT, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &new_request);
            
            *work_request = new_request;
        }
    }

    return next_task;
}

int get_stop(int stop, MPI_Request stop_request)
{
    if (!stop)
        MPI_Test(&stop_request, &stop, MPI_STATUS_IGNORE);

    return stop;
}

int *get_task()
{
    int ready;
    MPI_Request work_request;
    MPI_Isend(&ready, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, &work_request);

    int *task = allocate_mem(TASK_SIZE);
    MPI_Recv(task, TASK_SIZE, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Wait(&work_request, MPI_STATUS_IGNORE);

    return task;
}

void send_task(int node, int task, int *A)
{
    MPI_Send(&A[task * TASK_SIZE], TASK_SIZE, MPI_INT, node, WORK_TAG, MPI_COMM_WORLD);
}

void finish(MPI_Request *result_request, MPI_Request *work_request, int *A, int tasks_count, int next_task, int node_count)
{

    for (int i = 1; i < node_count; ++i)
    {
        int stop = 1;
        MPI_Request stop_request;
        MPI_Isend(&stop, 1, MPI_INT, i, STOP_TAG, MPI_COMM_WORLD, &stop_request);
    }

    get_results(result_request, node_count);
    distribute_work(work_request, A, tasks_count, next_task, node_count);
    free(A);
}

int *initialise(char init_mode)
{
    int *A = allocate_mem(N);
    init_mode == 'r' ? fill_random(A, N) : fill_ascending(A, N);
    return A;
}