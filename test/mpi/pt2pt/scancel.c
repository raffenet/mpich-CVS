/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Test of various send cancel calls";

int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size, source, dest;
    MPI_Comm      comm;
    MPI_Status    status;
    MPI_Request   req;
    static int bufsizes[4] = { 1, 100, 10000, 1000000 };
    char *buf;
    int veryPicky = 0;   /* Set to 1 to test "quality of implementation" in
			    a tricky part of cancel */
    int  cs, flag, n;

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    source = 0;
    dest   = size - 1;

    for (cs=0; cs<4; cs++) {
	if (rank == 0) {
	    n = bufsizes[cs];
	    buf = (char *)malloc( n );
	    if (!buf) {
		fprintf( stderr, "Unable to allocate %d bytes\n", n );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }
	    MPI_Isend( buf, n, MPI_CHAR, dest, cs, comm, &req );
	    MPI_Cancel( &req );
	    MPI_Wait( &req, &status );
	    MPI_Test_cancelled( &status, &flag );
	    if (!flag) {
		errs ++;
		printf( "Failed to cancel a Isend request\n" );
	    }
	    free( buf );
	}
	MPI_Barrier( comm );

	if (rank == 0) {
	    char *bsendbuf;
	    int bsendbufsize;
	    int bf, bs;
	    n = bufsizes[cs];
	    buf = (char *)malloc( n );
	    if (!buf) {
		fprintf( stderr, "Unable to allocate %d bytes\n", n );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }
	    bsendbufsize = n + MPI_BSEND_OVERHEAD;
	    bsendbuf = (char *)malloc( bsendbufsize );
	    if (!bsendbuf) {
		fprintf( stderr, "Unable to allocate %d bytes for bsend\n", n );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }
	    MPI_Buffer_attach( bsendbuf, bsendbufsize );
	    MPI_Ibsend( buf, n, MPI_CHAR, dest, cs, comm, &req );
	    MPI_Cancel( &req );
	    MPI_Wait( &req, &status );
	    MPI_Test_cancelled( &status, &flag );
	    if (!flag) {
		errs ++;
		printf( "Failed to cancel a Ibsend request\n" );
	    }
	    free( buf );
	    MPI_Buffer_detach( &bf, &bs );
	    free( bsendbuf );
	}
	MPI_Barrier( comm );

	/* We avoid ready send to self because an implementation
	   is free to detect the error in delivering a message to
	   itself without a pending receive; we could also check
	   for an error return from the MPI_Irsend */
	if (rank == 0 && dest != rank) {
	    n = bufsizes[cs];
	    buf = (char *)malloc( n );
	    if (!buf) {
		fprintf( stderr, "Unable to allocate %d bytes\n", n );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }
	    MPI_Irsend( buf, n, MPI_CHAR, dest, cs, comm, &req );
	    MPI_Cancel( &req );
	    MPI_Wait( &req, &status );
	    MPI_Test_cancelled( &status, &flag );
	    /* This can be pretty ugly.  The standard is clear (Section 3.8)
	       that either a sent message is received or the 
	       sent message is successfully cancelled.  Since this message
	       can never be received, the cancel must complete
	       successfully.  

	       However, since there is no matching receive, this
	       program is erroneous.  In this case, we can't really
	       flag this as an error */
	    if (!flag && veryPicky) {
		errs ++;
		printf( "Failed to cancel a Irsend request\n" );
	    }
	    free( buf );
	}
	MPI_Barrier( comm );

	if (rank == 0) {
	    n = bufsizes[cs];
	    buf = (char *)malloc( n );
	    if (!buf) {
		fprintf( stderr, "Unable to allocate %d bytes\n", n );
		MPI_Abort( MPI_COMM_WORLD, 1 );
	    }
	    MPI_Issend( buf, n, MPI_CHAR, dest, cs, comm, &req );
	    MPI_Cancel( &req );
	    MPI_Wait( &req, &status );
	    MPI_Test_cancelled( &status, &flag );
	    if (!flag) {
		errs ++;
		printf( "Failed to cancel a Issend request\n" );
	    }
	    free( buf );
	}
	MPI_Barrier( comm );

    }

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
