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
 * This program tests that MPI_Comm_split applies to intercommunicators;
 * this is an extension added in MPI-2
 */

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size, isLeft;
    MPI_Comm intercomm, newcomm;

    MTest_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if (size < 4) {
	printf( "This test requires at least 4 processes\n" );
	MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    while (MTestGetIntercomm( &intercomm, &isLeft, 2 )) {
	int key, color;
	/* Split this intercomm.  The new intercomms contain the 
	   processes that had odd (resp even) rank in their local group
	   in the original intercomm */
	MPI_Comm_rank( intercomm, &key );
	color = (key % 2);
	MPI_Comm_split( intercomm, color, key, &newcomm );
	/* Make sure that the new communicator has the appropriate pieces */
	if (newcomm != MPI_COMM_NULL) {
	    int orig_rsize, orig_size, new_rsize, new_size;
	    int predicted_size, flag;

	    MPI_Comm_test_inter( intercomm, &flag );
	    if (!flag) {
		errs++;
		printf( "Output communicator is not an intercomm\n" );
	    }

	    MPI_Comm_remote_size( intercomm, &orig_rsize );
	    MPI_Comm_remote_size( newcomm, &new_rsize );
	    MPI_Comm_size( intercomm, &orig_size );
	    MPI_Comm_size( newcomm, &new_size );
	    /* The local size is 1/2 the original size, +1 if the 
	       size was odd and the color was even */
	    predicted_size = orig_size / 2; 
	    if ((orig_size % 2) && !(key % 2)) predicted_size ++;
	    if (predicted_size != new_size) {
		errs++;
		printf( "Predicted size = %d but found %d for %s\n",
			predicted_size, new_size, MTestGetIntercommName() );
	    }
	    /* ... more to do */
	}
	MPI_Comm_free( &newcomm );
	MPI_Comm_free( &intercomm );
    }
    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
