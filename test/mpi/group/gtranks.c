#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    int errs = 0;
    int ranks[128], ranksout[128];
    MPI_Group gworld, gself;
    MPI_Comm  comm;
    int rank, size, i;

    MTest_Init( &argc, &argv );

    MPI_Comm_group( MPI_COMM_SELF, &gself );

    comm = MPI_COMM_WORLD;

    MPI_Comm_size( comm, &size );
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_group( comm, &gworld );
    for (i=0; i<size; i++) {
	ranks[i] = i;
	ranksout[i] = -1;
    }
    MPI_Group_translate_ranks( gworld, size, ranks, gself, ranksout );
    
    for (i=0; i<size; i++) {
	if (i == rank) {
	    if (ranksout[i] != 0) {
		errs++;
	    }
	}
	else {
	    if (ranksout[i] != MPI_UNDEFINED) {
		errs++;
	    }
	}
    }

    MTest_Finalize( errs );
    MPI_Finalize();

    return 0;
}
