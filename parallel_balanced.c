#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "parallel_balanced.h"
#include "mpi.h"

int main(int argc, char *argv[])
{
    const int N = 500;
    const int R = 100;

    int id, node_count;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &node_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    id == 0 ? master(N, R) : worker(N, R);

    return 0;
}

void master(int N, int R, int node_count)
{
    int counter = 0;
    int *A = initialise(N);
    int task_size = send_tasks(A, N, node_count);

    for (int i = 0; i < task_size; ++i)
    {
        counter += test(A[i]);
        counter += get_results();

        if (send_stop(counter >= R))
            break;
    }
}

void worker(int N, int R, int node_count, int id)
{
    int task_size = N / node_count;
    int *task = get_task(task_size);

    for (int i = 0; i < task_size; ++i)
    {
        send_result(test(task[i]));

        if (get_stop())
            break;
    }
}

int get_stop()
{
    int stop;
    MPI_Recv(&stop, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return stop;
}

int send_stop(int stop)
{
    for (int i = 1; i < node_count; ++i)
    {
        MPI_Send(&stop, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    return stop;
}

int get_results(int node_count)
{
    int result, counter = 0;

    for (int i = 1; i < node_count; ++i)
    {
        MPI_Recv(&result, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        counter += result;
    }

    return counter;
}

void send_result(result)
{
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
        MPI_Send(&A[master_task_size + (i - 1) * task_size], task_size, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    return master_task_size;
}

int *initialise(int N)
{
    int *A = allocate_mem(N);
    fill_random(A, N);
    return A;
}