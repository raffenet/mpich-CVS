#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    MPI_Comm dup, dupcopy;
    int ierr;
    
    MPI_Init( &argc, &argv );
    MPI_Comm_dup( MPI_COMM_WORLD, &dup );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    dupcopy = dup;
    MPI_Comm_free( &dupcopy );
    ierr = MPI_Barrier( dup );
    if (ierr == MPI_SUCCESS) {
	printf( "Returned wrong code in barrier\n" );
    }
    {
	int in, *input = &in;
	int out, *output = &out;
	ierr = MPI_Allgather(input, 1, MPI_INT, output, 1, MPI_INT, dup);
    }
    if (ierr == MPI_SUCCESS) {
	printf( "Returned wrong code in allgather\n" );
    }
    MPI_Finalize();
    return 0;
}
