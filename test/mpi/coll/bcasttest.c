#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>

#define NINTS     1123
#define ROOT      0
#define NUM_REPS  5

int main( int argc, char **argv)
{
    int *buf;
    int i, rank, reps;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    buf = (int *) malloc(NINTS*sizeof(int));
    
    for (reps=0; reps < NUM_REPS; reps++)
    {
	if (rank == ROOT)
	{
	    for (i=0; i<NINTS; i++) 
		buf[i] = i;
	}
	else
	{
	    for (i=0; i<NINTS; i++)
		buf[i] = -1;
	}
	
	MPI_Bcast(buf, NINTS, MPI_INT, ROOT, MPI_COMM_WORLD); 
	
	for (i=0; i<NINTS; i++)
	{
	    if (buf[i] != i)
		printf("Error: Rank=%d, i=%d, buf[i]=%d\n", rank, i, buf[i]);
	}
    }
    
    /* printf("Node %d done\n", rank); */
    if (rank == 0) 
	printf(" No Errors\n");

    fflush(stdout);
    
    MPI_Finalize();
    return 0;
}
