#include "mpi.h"

/* 
 * This is a special test to check that mpiexec handles the death of
 * some processes without an Abort or clean exit
 */
int main( int argc, char *argv[] )
{
    int rank;
    MPI_Init( 0, 0 );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if (rank > 0) {
	/* Cause some processes to exit */
	int *p =0 ;
	*p = rank;
    }
    MPI_Finalize( );
    return 0;
}
