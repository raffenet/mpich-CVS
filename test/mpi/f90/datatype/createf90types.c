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

static char MTEST_Descrip[] = "Test the routines to access the Fortran 90 datatypes from C";

/* Check the return from the routine */
static int checkType( const char str[], int p, int r, 
		      int err, MPI_Datatype dtype )
{
    int errs = 0;
    if (dtype == MPI_DATATYPE_NULL) {
	printf( "Unable to find a real type for (p=%d,r=%d) in %s\n", 
		p, r, str );
	errs++;
    }
    if (err) {
	errs++;
	MTestPrintError( err );
    }
    return errs;
}

int main( int argc, char *argv[] )
{
    int p, r;
    int errs = 0;
    int err;
    MPI_Datatype newtype;

    MPI_Init(0,0);

    /* Set the handler to errors return, since according to the
       standard, it is invalid to provide p and/or r that are unsupported */

    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    /* This should be a valid type similar to MPI_REAL */
    p = 3;
    r = 10;
    err = MPI_Type_create_f90_real( p, r, &newtype );
    errs += checkType( "REAL", p, r, err, newtype );

    r = MPI_UNDEFINED;
    err = MPI_Type_create_f90_real( p, r, &newtype );
    errs += checkType( "REAL", p, r, err, newtype );

    p = MPI_UNDEFINED;
    r = 10;
    err = MPI_Type_create_f90_real( p, r, &newtype );
    errs += checkType( "REAL", p, r, err, newtype );

    /* This should be a valid type similar to MPI_COMPLEX */
    p = 3;
    r = 10;
    err = MPI_Type_create_f90_complex( p, r, &newtype );
    errs += checkType( "COMPLEX", p, r, err, newtype );

    r = MPI_UNDEFINED;
    err = MPI_Type_create_f90_complex( p, r, &newtype );
    errs += checkType( "COMPLEX", p, r, err, newtype );

    p = MPI_UNDEFINED;
    r = 10;
    err = MPI_Type_create_f90_complex( p, r, &newtype );
    errs += checkType( "COMPLEX", p, r, err, newtype );

    /* This should be a valid type similar to MPI_INTEGER */
    p = 3;
    err = MPI_Type_create_f90_integer( p, &newtype );
    errs += checkType( "INTEGER", p, r, err, newtype );

    if (errs == 0) {
	printf( " No Errors\n" );
    }
    else {
	printf( " Found %d errors\n", errs );
    }

    MPI_Finalize();
    return 0;
}
