/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#include <pthread.h>

/* Make these global to simplify communicating them to the other threads */
MPI_Comm comm0, comm1, comm2, comm3;

#define MAXLOOP 200

int main( int argc, char **argv )
{
    int errs = 0;
    int rank, size, wrank, wsize, dest, a, b;
    int provided;
    MPI_Status status;

    MPI_Init_thread( &argc, &argv, MPI_THREAD_MULTIPLE, &provided );
    MTest_Init( &argc, &argv );
    if (provided != MPI_THREAD_MULTIPLE) {
	/* we don't have the support for this.  Exit with no errors */
	MTest_Finalize( 0 );
	MPI_Finalize();
	return 0;
    }

    /* Test of context id allocation in a multithreaded environment 
       Possible implementation errors that this test may find:
       1) Locks around an allreduce used to find a context id
          (test will hang)
       2) No locks or other thread-safety checks
          (multiple distinct communicators may get the same context id)

       Approach
       Create multiple communicators, one for each thread.
       Create threads (numbered 0 .. nthreads-1)
       Synchronize the processes
       Each thread then executes a Comm_dup() of its communicator,
       but after a delay that is determined by the rank of the process
       and the thread number.  
           This is done to allow threads corresponding to different 
	   communicators to enter the comm_dup function first

       After all threads have their new communicators, each thread sends its 
       thread number to its neighbor.  On receipt, each thread checks that 
       it recieved the correct number.

       Repeat the above a number of times to increase the chance that any
       race condition will be observed.
     */

    /* Use only 2 threads for now */
    MPI_Comm_dup( MPI_COMM_WORLD, &comm0 );
    MPI_Comm_dup( MPI_COMM_WORLD, &comm1 );
    
    /* FIXME: Still need to implement */
    MPI_Barrier( comm0 );

    /* create threads here */
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    for (i=0; i<MAXLOOP; i++) {
	MPI_Barrier( comm0 );
	
	if (!(wrank & 0x1)) {
	    /* wait a short while to let a different thread start first */
	    usleep( 16000 );
	}
	MPI_Dup( comm0, &comm2 );
	/* Check that we got the right communicator */
	MPI_Comm_free( &comm2 );
    }

    MTest_Finalize( errs );

    MPI_Finalize();

    return 0;
}


int duptest( void *extra_data )
{
    MPI_Comm comm2;
    int      i, wrank;

    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    for (i=0; i<MAXLOOP; i++) {
	MPI_Barrier( comm1 );
	
	if (wrank & 0x1) {
	    /* wait a short while to let a different thread start first */
	    usleep( 16000 );
	}
	MPI_Dup( comm1, &comm3 );
	/* Check that we got the right communicator */
	MPI_Comm_free( &comm3 );
    }

    return 0;
}
