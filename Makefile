EXECS=parallel_imbalanced parallel_balanced sequential
MPICC?=mpicc

all: ${EXECS}


parallel-imbalanced:parallel_imbalanced.c test_mpi.c
	${MPICC} -O2 -o parallel_imbalanced test_mpi.c parallel_imbalanced.c -lm

parallel-balanced:parallel_balanced.c test_mpi.c
	${MPICC} -O2 -o parallel_balanced test_mpi.c parallel_balanced.c -lm

sequential:sequential.c test_mpi.c
	${MPICC} -O2 -o sequential test_mpi.c sequential.c  -lm

clean:
	rm -f *.o ${EXECS} *~ *core
