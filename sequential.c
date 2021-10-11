#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "sequential.h"

int main(int argc, char *argv[])
{
    const int N = 500;
    const int R = 100;
    char init_mode = argv[1][0];
    int *A = initialise(N, init_mode);

    clock_t start, end;

    start = clock();
    int result = test_array(A, N, R);
    end = clock();

    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Execution time: %fs\n", cpu_time_used);

    return 0;
}

int *initialise(int N, char init_mode)
{
    int *A = allocate_mem(N);
    init_mode == 'r' ? fill_random(A, N) : fill_ascending(A, N);
    return A;
}

int test_array(int *A, int N, int R)
{
    int counter = 0;

    for (int i = 0; i < N; ++i)
    {
        if (test(A[i]) && ++counter >= R)
        {
            return 1;
        }
    }

    return 0;
}