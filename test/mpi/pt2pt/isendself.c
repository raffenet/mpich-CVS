#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int a[10], b[10], i;
    MPI_Status status;
    MPI_Request request;
    int rank;
    

    MPI_Init( 0, 0 );

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    for (i=0; i<10; i++) a[i] = i+1;

    MPI_Isend( a, 0, MPI_INT, rank, 0, MPI_COMM_WORLD, &request );
    MPI_Recv( b, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
	      &status );
    printf ("1 status = %d %d %d %d\n", status.MPI_SOURCE, status.MPI_TAG,
	    status.MPI_ERROR, status.count );
    /* printf( "b[0] = %d\n", b[0] );*/
    MPI_Wait( &request, &status );

    MPI_Isend( 0, 0, MPI_INT, rank, 0, MPI_COMM_WORLD, &request );
    MPI_Recv( 0, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
	      &status );
    printf ("2 status = %d %d %d %d\n", status.MPI_SOURCE, status.MPI_TAG,
	    status.MPI_ERROR, status.count );
    MPI_Wait( &request, &status );

    MPI_Finalize();
    return 0;
}
