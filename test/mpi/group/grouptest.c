/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>

/* This is a temporary test program that uses a special testing routine
   to create a group that contains more processes than are in comm_world,
   for ease of testing without requiring a parallel job run (since all
   group routines are local) 
*/
int main( int argc, char *argv[] )
{
    MPI_Group g1, g2;
    int ranks[16], size, rank, myrank;
    int errs = 0;

    MPI_Init(0,0);

#define MPIR_TEST_GROUPS
#ifdef MPIR_TEST_GROUPS
    /* Process for each rank in the group */
    for (myrank=0; myrank<16; myrank++) {
	MPITEST_Group_create( 16, myrank, &g1 ); 
#else
	MPI_Comm_group( MPI_COMM_WORLD, &g1 );
	MPI_Comm_rank( MPI_COMM_WORLD, &myrank );
#endif
	/* 16 members, this process is rank 0, return in group 1 */
	ranks[0] = 0; ranks[1] = 2; ranks[2] = 7;
	MPI_Group_incl( g1, 3, ranks, &g2 );
	
	/* Check the resulting group */
	MPI_Group_size( g2, &size );
	MPI_Group_rank( g2, &rank );
	
	if (size != 3) {
	    fprintf( stderr, "Size should be %d, is %d\n", 3, size );
	    errs++;
	}
	if (rank != 0) {
	    fprintf( stderr, "Rank should be %d, is %d\n", 0, rank );
	    errs++;
	}

	/* Add tests for additional group operations */
	/* 
	   g2 = incl 1,3,7
	   g3 = excl 1,3,7
	   intersect ( w, g2 ) => g2
	   intersect ( w, g3 ) => g3
	   intersect ( g2, g3 ) => empty
	   
	   g4 = rincl 1:n-1:2
	   g5 = rexcl 1:n-1:2
	   union( g4, g5 ) => world
	   g6 = rincl n-1:1:-1 
	   g7 = rexcl n-1:1:-1
	   union( g6, g7 ) => concat of entries, similar to world
	   diff( w, g2 ) => g3
	*/
#ifdef MPIR_TEST_GROUPS
        MPI_Group_free( &g1 );
    }
#endif
    
    if (errs == 0) {
	printf( "No errors\n" );
    }
    else {
	printf( "Found %d errors\n", errs );
    }
    MPI_Finalize();
    return 0;
}
