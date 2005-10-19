#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    MPI_Status status;
    int        err, errs = 0, len;
    char       msg[MPI_MAX_ERROR_STRING];

    MTest_Init( &argc, &argv );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    err = MPI_Probe( -80, 1, MPI_COMM_WORLD, &status );
    if (!err) {
	errs++;
	printf( "Did not detect an erroneous rank in MPI_Probe\n" );
    }
    else {
	/* Check that we can get a message for this error */
	/* (This works if it does not SEGV or hang) */
	MPI_Error_string( err, msg, &len );
    }

    MTest_Finalize( errs );
    MPI_Finalize( );
    return 0;
}
