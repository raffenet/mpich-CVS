/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

int main( int argc, char *argv[] )
{
    int errs = 0;
    int newrank, merr;

    MTest_Init( &argc, &argv );

    /* Graph map where there are no nodes for this process */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    merr = MPI_Graph_map( MPI_COMM_WORLD, 0, 0, 0, &newrank );
    if (merr) {
	errs++;
	printf( "Graph map returned an error\n" );
	MTestPrintError( merr );
    }
    if (newrank != MPI_UNDEFINED) {
	errs++;
	printf( "Graph map with no local nodes did not return MPI_UNDEFINED\n" );
    }
    
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
