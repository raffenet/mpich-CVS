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
    MPI_Comm comm, dupcomm;
    MPI_Request rreq[2];
    int count;
    int indicies[2];
    int flag, r1buf, r2buf, s1buf, s2buf;
    int rank;

    MTest_Init( &argc, &argv );
    
    while (MTestGetIntercomm( &comm, 2 )) {
	if (comm == MPI_COMM_NULL) continue;

	MPI_Comm_dup( comm, &dupcomm );
	
	/* Check that there are separate contexts.  We do this by setting
	   up nonblocking received on both communicators, and then
	   sending to them.  If the contexts are different, tests on the
	   unstaisfied communicator should indicate no available message */
	MPI_Comm_rank( comm, &rank );
	if (rank == 0) {
	    s1buf = 456;
	    s2buf = 17;
	    r1buf = r2buf = -1;
	    MPI_Irecv( &r1buf, 1, MPI_INT, 0, 0, dupcomm, &rreq[0] );
	    MPI_Irecv( &r2buf, 1, MPI_INT, 0, 0, comm, &rreq[1] );
	    MPI_Send( &s2buf, 1, MPI_INT, 0, 0, comm );
	    MPI_Waitsome(2, rreq, &count, indicies, MPI_STATUSES_IGNORE);
	    if (count == 1 && indicies[0] != 1) {
		errs++;
		printf( "Error in context values for intercomm\n" );
	    }
	    MPI_Send( &s1buf, 1, MPI_INT, 0, 0, dupcomm );
	    MPI_Wait( &rreq[0], MPI_STATUS_IGNORE );
	    if (r1buf != s1buf) {
		errs++;
		printf( "Wrong value in communication on dupcomm %d != %d\n",
			r1buf, s1buf );
	    }
	    if (r2buf != s2buf) {
		errs++;
		printf( "Wrong value in communication on comm %d != %d\n",
			r2buf, s2buf );
	    }
	}
	MPI_Comm_free( &dupcomm );
	MPI_Comm_free( &comm );
    }
    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
  
}
