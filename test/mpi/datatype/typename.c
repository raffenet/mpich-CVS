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
    MPI_Type_get_name( MPI_DOUBLE, name, &namelen );
    if (strncmp( name, "double", MPI_MAX_OBJECT_NAME )) {
	errs++;
	fprintf( stderr, "Expected double but got :%s:\n", name );
    }

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
