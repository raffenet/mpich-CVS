/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Test MPI_Allreduce with non-commutative user-define operations";

/* This implements a simple matrix-matrix multiply.  This is an associative
   but not commutative operation.  The matrix size is set in matSize;
   the number of matrices is the count argument. The matrix is stored
   in C order, so that
     c(i,j) is cin[j+i*matSize]
 */
#define MAXCOL 256
static int matSize = 0;  /* Must be < MAXCOL */
void uop( void *cinPtr, void *coutPtr, int *count, MPI_Datatype *dtype )
{
    const int *cin = (const int *)cinPtr;
    int *cout = (int *)coutPtr;
    int i, j, k, nmat;
    int tempcol[MAXCOL];

    for (nmat = 0; nmat < *count; nmat++) {
	for (j=0; j<matSize; j++) {
	    for (i=0; i<matSize; i++) {
		tempcol[i] = 0;
		for (k=0; k<matSize; k++) {
		    /* col[i] += cin(i,k) * cout(k,j) */
		    tempcol[i] += cin[k+i*matSize] * cout[j+k*matSize];
		}
	    }
	    for (i=0; i<matSize; i++) {
		cout[j+i*matSize] = tempcol[i];
	    }
	}
    }
}

/* Initialize the integer matrix as a permutation of rank with rank+1.
   If we call this matrix P_r, we know that product of P_0 P_1 ... P_{size-1}
   is the identity I.
*/   

static void initMat( MPI_Comm comm, int mat[] )
{
    int i, size, rank;
    
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    for (i=0; i<size*size; i++) mat[i] = 0;

    for (i=0; i<size; i++) {
	if (i == rank)                   mat[((i+1)%size) + i * size] = 1;
	else if (i == ((rank + 1)%size)) mat[((i+size-1)%size) + i * size] = 1;
	else                             mat[i+i*size] = 1;
    }
}

/* Compare a matrix with the identity matrix */
static int isIdentity( MPI_Comm comm, int mat[] )
{
    int i, j, size, rank, errs = 0;
    
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    for (i=0; i<size; i++) {
	for (j=0; j<size; j++) {
	    if (i == j) {
		if (mat[j+i*size] != 1) {
		    errs++;
		}
	    }
	    else {
		if (mat[j+i*size] != 0) {
		    errs++;
		}
	    }
	}
    }
    return errs;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    int size;
    int minsize = 2, count; 
    MPI_Comm      comm;
    int *buf, *bufout;
    MPI_Op op;
    MPI_Datatype mattype;

    MTest_Init( &argc, &argv );

    MPI_Comm_size( MPI_COMM_WORLD, &matSize );

    MPI_Op_create( uop, 0, &op );
    
    while (MTestGetIntracommGeneral( &comm, minsize, 1 )) {
	if (comm == MPI_COMM_NULL) continue;
	MPI_Comm_size( comm, &size );

	/* Only one matrix for now */
	count = 1;

	/* A single matrix, the size of the communicator */
	MPI_Type_contiguous( size*size, MPI_INT, &mattype );
	MPI_Type_commit( &mattype );
	
	buf = (int *)malloc( count * size * size * sizeof(int) );
	if (!buf) MPI_Abort( MPI_COMM_WORLD, 1 );
	bufout = (int *)malloc( count * size * size * sizeof(int) );
	if (!bufout) MPI_Abort( MPI_COMM_WORLD, 1 );

	initMat( comm, buf );
	MPI_Allreduce( buf, bufout, count, mattype, op, comm );
	errs += isIdentity( comm, bufout );

	/* Try the same test, but using MPI_IN_PLACE */
	initMat( comm, bufout );
	MPI_Allreduce( MPI_IN_PLACE, bufout, count, mattype, op, comm );
	errs += isIdentity( comm, bufout );

	free( buf );
	free( bufout );

	MTestFreeComm( &comm );
    }

    MPI_Op_free( &op );

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
