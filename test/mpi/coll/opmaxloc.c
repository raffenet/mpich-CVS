/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>
#include "mpitest.h"

static char MTEST_Descrip[] = "Test MPI_MAXLOC operations on datatypes dupported by MPICH2";

/*
 * This test looks at the handling of char and types that  are not required 
 * integers (e.g., long long).  MPICH2 allows
 * these as well.  A strict MPI test should not include this test.
 */
int main( int argc, char *argv[] )
{
    int errs = 0;
    int rank, size;
    MPI_Comm      comm;

    MTest_Init( &argc, &argv );

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank( comm, &rank );
    MPI_Comm_size( comm, &size );

    /* 2 int */
    {
	struct twoint { int val; int loc; } cinbuf[3], coutbuf[3];
 	
	cinbuf[0].val = 1;
	cinbuf[0].loc = rank;
	cinbuf[1].val = 0;
	cinbuf[1].loc = rank;
	cinbuf[2].val = (rank & 0x7f);
	cinbuf[2].loc = rank;
	
	coutbuf[0].val = 0;
	coutbuf[0].loc = -1;
	coutbuf[1].val = 1;
	coutbuf[1].loc = -1;
	coutbuf[2].val = 1;
	coutbuf[2].loc = -1;
	MPI_Reduce( cinbuf, coutbuf, 3, MPI_2INT, MPI_MAXLOC, 0, comm );
	if (rank == 0) {
	    if (coutbuf[0].val != 1 && coutbuf[0].loc != -1) {
		errs++;
		fprintf( stderr, "2int MAXLOC(1) test failed\n" );
	    }
	    if (coutbuf[1].val != 0 && coutbuf[1].loc != -1) {
		errs++;
		fprintf( stderr, "2int MAXLOC(0) test failed\n" );
	    }
	    if (coutbuf[2].val != size-1 && coutbuf[2].loc != size-1) {
		errs++;
		fprintf( stderr, "2int MAXLOC(>) test failed\n" );
	    }
	}
    }

    /* float int */
    {
	struct floatint { float val; int loc; } cinbuf[3], coutbuf[3];
 	
	cinbuf[0].val = 1;
	cinbuf[0].loc = rank;
	cinbuf[1].val = 0;
	cinbuf[1].loc = rank;
	cinbuf[2].val = (float)rank;
	cinbuf[2].loc = rank;
	
	coutbuf[0].val = 0;
	coutbuf[0].loc = -1;
	coutbuf[1].val = 1;
	coutbuf[1].loc = -1;
	coutbuf[2].val = 1;
	coutbuf[2].loc = -1;
	MPI_Reduce( cinbuf, coutbuf, 3, MPI_FLOAT_INT, MPI_MAXLOC, 0, comm );
	if (rank == 0) {
	    if (coutbuf[0].val != 1 && coutbuf[0].loc != -1) {
		errs++;
		fprintf( stderr, "float-int MAXLOC(1) test failed\n" );
	    }
	    if (coutbuf[1].val != 0 && coutbuf[1].loc != -1) {
		errs++;
		fprintf( stderr, "float-int MAXLOC(0) test failed\n" );
	    }
	    if (coutbuf[2].val != size-1 && coutbuf[2].loc != size-1) {
		errs++;
		fprintf( stderr, "float-int MAXLOC(>) test failed\n" );
	    }
	}
    }
    
    /* long int */
    {
	struct longint { long val; int loc; } cinbuf[3], coutbuf[3];
 	
	cinbuf[0].val = 1;
	cinbuf[0].loc = rank;
	cinbuf[1].val = 0;
	cinbuf[1].loc = rank;
	cinbuf[2].val = rank;
	cinbuf[2].loc = rank;
	
	coutbuf[0].val = 0;
	coutbuf[0].loc = -1;
	coutbuf[1].val = 1;
	coutbuf[1].loc = -1;
	coutbuf[2].val = 1;
	coutbuf[2].loc = -1;
	MPI_Reduce( cinbuf, coutbuf, 3, MPI_LONG_INT, MPI_MAXLOC, 0, comm );
	if (rank == 0) {
	    if (coutbuf[0].val != 1 && coutbuf[0].loc != -1) {
		errs++;
		fprintf( stderr, "long-int MAXLOC(1) test failed\n" );
	    }
	    if (coutbuf[1].val != 0 && coutbuf[1].loc != -1) {
		errs++;
		fprintf( stderr, "long-int MAXLOC(0) test failed\n" );
	    }
	    if (coutbuf[2].val != size-1 && coutbuf[2].loc != size-1) {
		errs++;
		fprintf( stderr, "long-int MAXLOC(>) test failed\n" );
	    }
	}
    }

    /* short int */
    {
	struct shortint { short val; int loc; } cinbuf[3], coutbuf[3];
 	
	cinbuf[0].val = 1;
	cinbuf[0].loc = rank;
	cinbuf[1].val = 0;
	cinbuf[1].loc = rank;
	cinbuf[2].val = rank;
	cinbuf[2].loc = rank;
	
	coutbuf[0].val = 0;
	coutbuf[0].loc = -1;
	coutbuf[1].val = 1;
	coutbuf[1].loc = -1;
	coutbuf[2].val = 1;
	coutbuf[2].loc = -1;
	MPI_Reduce( cinbuf, coutbuf, 3, MPI_SHORT_INT, MPI_MAXLOC, 0, comm );
	if (rank == 0) {
	    if (coutbuf[0].val != 1 && coutbuf[0].loc != -1) {
		errs++;
		fprintf( stderr, "short-int MAXLOC(1) test failed\n" );
	    }
	    if (coutbuf[1].val != 0 && coutbuf[1].loc != -1) {
		errs++;
		fprintf( stderr, "short-int MAXLOC(0) test failed\n" );
	    }
	    if (coutbuf[2].val != size-1 && coutbuf[2].loc != size-1) {
		errs++;
		fprintf( stderr, "short-int MAXLOC(>) test failed\n" );
	    }
	}
    }
    
    /* double int */
    {
	struct doubleint { double val; int loc; } cinbuf[3], coutbuf[3];
 	
	cinbuf[0].val = 1;
	cinbuf[0].loc = rank;
	cinbuf[1].val = 0;
	cinbuf[1].loc = rank;
	cinbuf[2].val = rank;
	cinbuf[2].loc = rank;
	
	coutbuf[0].val = 0;
	coutbuf[0].loc = -1;
	coutbuf[1].val = 1;
	coutbuf[1].loc = -1;
	coutbuf[2].val = 1;
	coutbuf[2].loc = -1;
	MPI_Reduce( cinbuf, coutbuf, 3, MPI_DOUBLE_INT, MPI_MAXLOC, 0, comm );
	if (rank == 0) {
	    if (coutbuf[0].val != 1 && coutbuf[0].loc != -1) {
		errs++;
		fprintf( stderr, "double-int MAXLOC(1) test failed\n" );
	    }
	    if (coutbuf[1].val != 0 && coutbuf[1].loc != -1) {
		errs++;
		fprintf( stderr, "double-int MAXLOC(0) test failed\n" );
	    }
	    if (coutbuf[2].val != size-1 && coutbuf[2].loc != size-1) {
		errs++;
		fprintf( stderr, "double-int MAXLOC(>) test failed\n" );
	    }
	}
    }
    
#ifdef HAVE_LONG_DOUBLE
    /* long double int */
    {
	struct longdoubleint { long double val; int loc; } cinbuf[3], coutbuf[3];
 	
	cinbuf[0].val = 1;
	cinbuf[0].loc = rank;
	cinbuf[1].val = 0;
	cinbuf[1].loc = rank;
	cinbuf[2].val = rank;
	cinbuf[2].loc = rank;
	
	coutbuf[0].val = 0;
	coutbuf[0].loc = -1;
	coutbuf[1].val = 1;
	coutbuf[1].loc = -1;
	coutbuf[2].val = 1;
	coutbuf[2].loc = -1;
	MPI_Reduce( cinbuf, coutbuf, 3, MPI_LONG_DOUBLE_INT, MPI_MAXLOC, 0, comm );
	if (rank == 0) {
	    if (coutbuf[0].val != 1 && coutbuf[0].loc != -1) {
		errs++;
		fprintf( stderr, "long double-int MAXLOC(1) test failed\n" );
	    }
	    if (coutbuf[1].val != 0 && coutbuf[1].loc != -1) {
		errs++;
		fprintf( stderr, "long double-int MAXLOC(0) test failed\n" );
	    }
	    if (coutbuf[2].val != size-1 && coutbuf[2].loc != size-1) {
		errs++;
		fprintf( stderr, "long double-int MAXLOC(>) test failed\n" );
	    }
	}
    }
#endif

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}
