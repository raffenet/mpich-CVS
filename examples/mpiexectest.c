/* simple test for multiple executables */
#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[])
{
    int  i, myid, numprocs;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
    MPI_Comm_rank( MPI_COMM_WORLD, &myid );
    MPI_Get_processor_name( processor_name, &namelen );

    fprintf( stdout, "[%d] Process %d of %d (%s) is on %s\n",
	     myid, myid, numprocs, argv[0], processor_name );
    fflush( stdout );

    for ( i = 1; i < argc; i++ ) {
	fprintf( stdout, "[%d] argv[%d]=\"%s\"\n", myid, i, argv[i] ); 
	fflush( stdout );
    }

    MPI_Finalize( );
}
