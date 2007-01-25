/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
 * This program tests that MPI_Comm_create applies to intercommunicators;
 * this is an extension added in MPI-2
 */

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, isLeft;
    MPI_Comm intercomm, newcomm;
    MPI_Group oldgroup, newgroup;
    
    MTest_Init( &argc, &argv );
    
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (size < 4) {
	printf( "This test requires at least 4 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    while (MTestGetIntercomm( &intercomm, &isLeft, 2 )) {
	int ranks[10], nranks;
        MPI_Comm_group( intercomm, &oldgroup );
	ranks[0] = 0;
	nranks   = 1;
	MPI_Group_incl( oldgroup, nranks, ranks, &newgroup );
	MPI_Comm_create( intercomm, newgroup, &newcomm );
	/* Make sure that the new communicator has the appropriate pieces */
	if (newcomm != MPI_COMM_NULL) {
	    int orig_rsize, orig_size, new_rsize, new_size;
	    int predicted_size, flag;

	    MPI_Comm_test_inter( intercomm, &flag );
	    if (!flag) {
		errs++;
		printf( "Output communicator is not an intercomm\n" );
	    }
	    continue;

	    MPI_Comm_remote_size( newcomm, &new_rsize );
	    MPI_Comm_size( newcomm, &new_size );
	    /* The new communicator has 1 process in each group */
	    if (new_rsize != 1) {
		errs++;
		printf( "Remote size is %d, should be one\n", new_rsize );
	    }
	    if (new_size != 1) {
		errs++;
		printf( "Local size is %d, should be one\n", new_size );
	    }
	    /* ... more to do */
	}
	MPI_Group_free( &oldgroup );
	MPI_Group_free( &newgroup );
	if (newcomm != MPI_COMM_NULL) {
	    MPI_Comm_free( &newcomm );
	}
	MPI_Comm_free( &intercomm );
    }

    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
