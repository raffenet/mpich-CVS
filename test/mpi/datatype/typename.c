/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv )
{
    char name[MPI_MAX_OBJECT_NAME];
    int namelen;
    int errs = 0;

    MPI_Init(0,0);
    
    /* Sample some datatypes */
    /* See 8.4, "Naming Objects" in MPI-2.  The default name is the same
       as the datatype name */
    MPI_Type_get_name( MPI_DOUBLE, name, &namelen );
    if (strncmp( name, "MPI_DOUBLE", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected MPI_DOUBLE but got :%s:\n", name );
    }

    MPI_Type_get_name( MPI_INT, name, &namelen );
    if (strncmp( name, "MPI_INT", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected MPI_INT but got :%s:\n", name );
    }

    /* Try resetting the name */
    MPI_Type_set_name( MPI_INT, "int" );
    name[0] = 0;
    MPI_Type_get_name( MPI_INT, name, &namelen );
    if (strncmp( name, "int", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected int but got :%s:\n", name );
    }

    if (errs) {
	fprintf( stderr, "Found %d errors\n", errs );
    }
    else {
	printf( "No errors\n" );
    }
    MPI_Finalize();
    return 0;
}
