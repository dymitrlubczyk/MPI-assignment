EXECS=parallel_imbalanced parallel_balanced sequential
MPICC?=mpicc

all: ${EXECS}


test_mpi:test_mpi.c
	${MPICC} -O2 -o test_mpi test_mpi.c -lm -std=c99 


clean:
	rm -f *.o ${EXECS} *~ *core

