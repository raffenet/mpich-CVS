#include "mpi.h" 
#include "stdio.h"

/* tests a series of puts, gets, and accumulate on 2 processes */

#define SIZE 10

int main(int argc, char *argv[]) 
{ 
    int rank, nprocs, A[SIZE], B[SIZE], i;
    MPI_Win win;
 
    MPI_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    if (rank == 0) {
        for (i=0; i<SIZE; i++)
            A[i] = B[i] = i;
    }
    else {
        for (i=0; i<SIZE; i++) {
            A[i] = (-3)*i;
            B[i] = (-4)*i;
        }
    }

    MPI_Win_create(B, SIZE*sizeof(int), sizeof(int), MPI_INFO_NULL, 
                   MPI_COMM_WORLD, &win); 

    MPI_Win_fence(0, win); 

    if (rank == 0) {
        for (i=0; i<SIZE-1; i++)
            MPI_Put(A+i, 1, MPI_INT, 1, i, 1, MPI_INT, win);
    }        
    else {
        for (i=0; i<SIZE-1; i++)
            MPI_Get(A+i, 1, MPI_INT, 0, i, 1, MPI_INT, win);

        MPI_Accumulate(A+i, 1, MPI_INT, 0, i, 1, MPI_INT, MPI_SUM, win);
    }
    MPI_Win_fence(0, win); 

    if (rank == 1) {
        for (i=0; i<SIZE-1; i++) {
            if (A[i] != B[i])
                printf("Put/Get Error: A[i]=%d, B[i]=%d\n", A[i], B[i]);
        }
    }
    else {
        if (B[SIZE-1] != SIZE - 1 - 3*(SIZE-1)) 
            printf("Accumulate Error: B[SIZE-1] is %d, should be %d\n", B[SIZE-1], SIZE - 1 - 3*(SIZE-1));
    }

    MPI_Win_free(&win); 
    if (rank==0) printf("Done\n");
    MPI_Finalize(); 
    return 0; 
} 
