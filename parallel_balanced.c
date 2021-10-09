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

    id == 0 ? master(N, R, node_count) : worker(N, R, node_count, id);
    
    MPI_Finalize();

    return 0;
}

void master(int N, int R, int node_count)
{
    double start, end;
    int counter = 0;
    int *A = initialise(N);
    
    start = MPI_Wtime();

    int task_size = send_tasks(A, N, node_count);

    printf("Master starts");
    for (int i = 0; i < task_size; ++i)
    {
        counter += test(A[i]);
        counter += get_results(node_count);

        if (send_stop(counter >= R, node_count))
            break;
    }

    end = MPI_Wtime();
    printf("Master ends, execution time: %fs\n", end-start);
}

void worker(int N, int R, int node_count, int id)
{
    printf("Node %d starts", id);

    int task_size = N / node_count;
    int *task = get_task(task_size);

    for (int i = 0; i < task_size; ++i)
    {
        send_result(test(task[i]));

        if (get_stop())
            break;
    }

    printf("Node %d ends", id);
}

int get_stop()
{
    int stop;
    MPI_Recv(&stop, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return stop;
}

int send_stop(int stop, int node_count)
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

void send_result(int result)
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
    printf("Master sends tasks");
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