#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    MPI_Status status;
    int        err, errs = 0, len;
    int        buf[1];
    char       msg[MPI_MAX_ERROR_STRING];

    MTest_Init( &argc, &argv );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    buf[0] = 1;
    err = MPI_Allreduce( buf, buf, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (!err) {
	errs++;
	printf( "Did not detect aliases arguments in MPI_Allreduce\n" );
    }
    else {
	/* Check that we can get a message for this error */
	/* (This works if it does not SEGV or hang) */
	MPI_Error_string( err, msg, &len );
    }
    err = MPI_Reduce( buf, buf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    if (!err) {
	errs++;
	printf( "Did not detect aliases arguments in MPI_Reduce\n" );
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
