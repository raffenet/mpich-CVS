/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

/* This example sends a vector and receives individual elements, but the
   root process does not receive any data */

int main( int argc, char **argv )
{
    MPI_Datatype vec;
    double *vecin, *vecout, ivalue;
    int    root, i, n, stride, err = 0;
    int    rank, size;

    MTest_Init( &argc, &argv );
    
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    n = 12;
    stride = 10;
    vecin = (double *)malloc( n * stride * size * sizeof(double) );
    vecout = (double *)malloc( n * sizeof(double) );
    
    MPI_Type_vector( n, 1, stride, MPI_DOUBLE, &vec );
    MPI_Type_commit( &vec );

    for (i=0; i<n*stride*size; i++) vecin[i] = (double)i;
    for (root=0; root<size; root++) {
	for (i=0; i<n; i++) vecout[i] = -1.0;
	if (rank == root) {
	    MPI_Scatter( vecin, 1, vec, MPI_IN_PLACE, -1, MPI_TYPE_NULL, root, 
			 MPI_COMM_WORLD );
	}
	else {
	    MPI_Scatter( NULL, -1, MPI_TYPE_NULL, vecout, n, MPI_DOUBLE, root, 
			 MPI_COMM_WORLD );
	    ivalue = rank * ((n-1) * stride + 1);
	    for (i=0; i<n; i++) {
		if (vecout[i] != ivalue) {
		    printf( "Expected %f but found %f\n", 
			    ivalue, vecout[i] );
		    err++;
		}
		ivalue += stride;
	    }
	}
    }
    
    MTest_Finalize( err );
    MPI_Type_free( &vec );
    MPI_Finalize();
    return 0;
}

