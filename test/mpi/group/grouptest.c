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
    int ranks[16], size, rank;
    int errs = 0;

    MPI_Init(0,0);

    MPITEST_Group_create( 16, 0, &g1 ); 
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
    
    if (errs == 0) {
	printf( "No errors\n" );
    }
    else {
	printf( "Found %d errors\n", errs );
    }
    MPI_Finalize();
    return 0;
}
