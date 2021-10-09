#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "test_mpi.h"
#include "sequential.h"

int main(int argc, char *argv[])
{
    const int N = 500;
    const int R = 1;
    int* A = initialise(N);
    

    clock_t start, end;

    start = clock();
    int result = test_array(A,N,R);
    end = clock();

    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    if(result)
        printf("Success in: %fs\n", cpu_time_used);

    else
       printf("Failier %fs\n", cpu_time_used); 
    
    return 0;
}

int* initialise(int N){
    int* A = allocate_mem(N);
    fill_random(A, N);
    return A;
}

int test_array(int* A, int N ,int R){
    int c = 0;

    for(int i=0; i<N; ++i){
        if(test(A[i]) && ++c==R){
            return 1;
        }
    }

    return 0;
}