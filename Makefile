EXECS=parallel-imbalanced parallel-balanced sequential
MPICC?=mpicc

all: ${EXECS}


parallel-imbalanced:parallel-imbalanced.c test_mpi.c
	${MPICC} -Wall -O3 -o parallel-imbalanced test_mpi.c parallel-imbalanced.c -lm

parallel-balanced:parallel-balanced.c test_mpi.c
	${MPICC} -Wall -O3 -o parallel-balanced test_mpi.c parallel-balanced.c -lm

sequential:sequential.c test_mpi.c
	${MPICC} -Wall -O3 -o sequential test_mpi.c sequential.c  -lm

clean:
	rm -f *.o ${EXECS} *~ *core
