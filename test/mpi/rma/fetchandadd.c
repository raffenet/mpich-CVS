#include "mpi.h" 
#include "stdio.h"
#include "stdlib.h"

/* Fetch and add example from Using MPI-2 (the non-scalable version,
   Fig. 6.12). */ 


#define NTIMES 10  /* no of times each process calls the counter
                      routine */

int localvalue=0;  /* contribution of this process to the counter. We
                    define it as a global variable because attribute
                    caching on the window is not enabled yet. */ 

void Get_nextval(MPI_Win win, int *val_array, MPI_Datatype get_type,
                 int rank, int nprocs, int *value);

int main(int argc, char *argv[]) 
{ 
    int rank, nprocs, i, blens[2], disps[2], *counter_mem, *val_array,
        value;
    MPI_Datatype get_type;
    MPI_Win win;
 
    MPI_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

    if (rank == 0) {
        /* allocate counter memory and initialize to 0 */
        counter_mem = (int *) calloc(nprocs, sizeof(int));
        MPI_Win_create(counter_mem, nprocs*sizeof(int), sizeof(int),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        for (i=0; i<300; i++) {
            usleep(1000);
            MPIDI_CH3I_Progress(0);
        }

        MPI_Win_free(&win); 
        free(counter_mem);
    }
    else {
        blens[0] = rank;
        disps[0] = 0;
        blens[1] = nprocs - rank - 1;
        disps[1] = rank + 1;

        MPI_Type_indexed(2, blens, disps, MPI_INT, &get_type);
        MPI_Type_commit(&get_type);

        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win); 

        val_array = (int *) malloc(nprocs * sizeof(int));

        for (i=0; i<NTIMES; i++) {
            Get_nextval(win, val_array, get_type, rank, nprocs, &value);
            printf("Rank %d, counter %d\n", rank, value);
        }

        MPI_Win_free(&win);
        free(val_array);
        MPI_Type_free(&get_type);
    }

    MPI_Finalize(); 
    return 0; 
} 


void Get_nextval(MPI_Win win, int *val_array, MPI_Datatype get_type,
                 int rank, int nprocs, int *value) 
{
    int one=1, i;

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
    MPI_Accumulate(&one, 1, MPI_INT, 0, rank, 1, MPI_INT, MPI_SUM, win);
    MPI_Get(val_array, 1, get_type, 0, 0, 1, get_type, win); 
    MPI_Win_unlock(0, win);

    *value = 0;
    val_array[rank] = localvalue;
    for (i=0; i<nprocs; i++)
        *value = *value + val_array[i];

    localvalue++;
}
