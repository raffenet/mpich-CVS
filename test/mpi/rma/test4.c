#include "mpi.h" 
#include "stdio.h"

/* tests passive target RMA on 2 processes. This test DOES NOT RUN
   currently because of lack of thread-safety in the progress engine. */

#define SIZE1 10
#define SIZE2 20

int main(int argc, char *argv[]) 
{ 
    int rank, nprocs, A[SIZE2], B[SIZE2], i;
    MPI_Win win;
 
    MPI_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (nprocs != 2) {
        printf("Run this program with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    if (rank == 0) {
        for (i=0; i<SIZE2; i++) A[i] = B[i] = i;
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win); 
        MPI_Win_lock(MPI_LOCK_SHARED, 1, 0, win);
        for (i=0; i<SIZE1; i++)
            MPI_Put(A+i, 1, MPI_INT, 1, i, 1, MPI_INT, win);
        for (i=0; i<SIZE1; i++)
            MPI_Get(B+i, 1, MPI_INT, 1, SIZE1+i, 1, MPI_INT, win);
        MPI_Win_unlock(1, win);
        MPI_Win_free(&win);

        for (i=0; i<SIZE1; i++) 
            if (B[i] != (-4)*(i+SIZE1)) 
                printf("Get Error: B[i] is %d, should be %d\n", B[i], (-4)*(i+SIZE1));
    }
 
    else {  /* rank=1 */
        for (i=0; i<SIZE2; i++) B[i] = (-4)*i;
        MPI_Win_create(B, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, 
                       MPI_COMM_WORLD, &win);
        MPI_Win_free(&win); 
        
        for (i=0; i<SIZE1; i++) {
            if (B[i] != i)
                printf("Put Error: B[i] is %d, should be %d\n", B[i], i);
        }
    }

    if (rank==0) printf("Done\n");
    MPI_Finalize(); 
    return 0; 
} 
