#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    int ierr, errs=0;
    MPI_Comm newcomm;

    MPI_Init( &argc, &argv );

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    ierr = MPI_Comm_connect( "myhost:27", MPI_INFO_NULL, 0, 
			     MPI_COMM_WORLD, &newcomm );
    if (ierr == MPI_SUCCESS) {
	errs++;
	printf( "Comm_connect returned success with bogus port\n" );
	MPI_Comm_free( &newcomm );
    }
    else {
	char str[MPI_MAX_ERROR_STRING];
	int  slen;
	/* Check the message */
	MPI_Error_string( ierr, str, &slen );
	printf( "Error message is: %s\n", str );
    }
    fflush(stdout);
    MPI_Finalize();

    return 0;
}
