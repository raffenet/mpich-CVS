/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <iostream>
#include "mpitestcxx.h"

static char MTEST_Descrip[] = "Simple intercomm barrier test";

/* This only checks that the Barrier operation accepts intercommunicators.
   It does not check for the semantics of a intercomm barrier (all processes
   in the local group can exit when (but not before) all processes in the 
   remote group enter the barrier */
int main( int argc, char *argv[] )
{
    int errs = 0;
    int leftGroup;
    MPI::Intercomm comm;
    MPI::Datatype datatype;

    MTest_Init( );

    datatype = MPI::INT;
    while (MTestGetIntercomm( comm, leftGroup, 4 )) {
	/* Get an intercommunicator */
	if (leftGroup) {
	    comm.Barrier( );
	}
	else {
	    /* In the right group */
	    comm.Barrier();
	}
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
