EXECS=parallel_imbalanced parallel_balanced sequential
MPICC?=mpicc

all: ${EXECS}


test_mpi:test_mpi.c
	${MPICC} -O2 -o test_mpi test_mpi.c -lm -std=c99 

# parallel_imbalanced:parallel_imbalanced.c test_mpi.c
# 	${MPICC} -O2 -o parallel_imbalanced test_mpi.c parallel_imbalanced.c -lm -std=c99 

# parallel_balanced:parallel_balanced.c test_mpi.c
# 	${MPICC} -O2 -o parallel_balanced test_mpi.c parallel_balanced.c -lm -std=c99 

# sequential:sequential.c test_mpi.c
# 	${MPICC} -O2 -o sequential test_mpi.c sequential.c -lm -std=c99 

clean:
	rm -f *.o ${EXECS} *~ *core

