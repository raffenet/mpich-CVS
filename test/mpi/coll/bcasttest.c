#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>

#define ROOT      0
#define NUM_REPS  5
#define NUM_SIZES 3

int main( int argc, char **argv)
{
    int *buf;
    int i, rank, reps, n;
    int sizes[NUM_SIZES] = { 100, 64*1024, 128*1024 };
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    buf = (int *) malloc(sizes[2]*sizeof(int));

    for (n=0; n<NUM_REPS; n++)
    {
	if (rank == ROOT)
	{
	    printf("bcasting %d bytes\n", sizes[n]);
	    fflush(stdout);
	}
	for (reps=0; reps < NUM_REPS; reps++)
	{
	    if (rank == ROOT)
	    {
		for (i=0; i<sizes[n]; i++) 
		    buf[i] = i;
		printf("%d,", reps);
		fflush(stdout);
	    }
	    else
	    {
		for (i=0; i<sizes[n]; i++)
		    buf[i] = -1;
	    }

	    MPI_Bcast(buf, sizes[n], MPI_INT, ROOT, MPI_COMM_WORLD); 

	    for (i=0; i<sizes[n]; i++)
	    {
		if (buf[i] != i)
		    printf("Error: Rank=%d, i=%d, buf[i]=%d\n", rank, i, buf[i]);
	    }
	}
	if (rank == ROOT)
	{
	    printf("\n");
	    fflush(stdout);
	}
    }
    
    /* printf("Node %d done\n", rank); */
    if (rank == 0) 
	printf(" No Errors\n");

    fflush(stdout);

    free(buf);

    MPI_Finalize();
    return 0;
}
