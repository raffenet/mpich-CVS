#include "cross_conf.h"

#ifdef F77_NAME_UPPER
#define flogical_ FLOGICAL
#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
#define flogical_ flogical
#endif

#define CROSS_SPEC_OUTPUT  "cross_spec.txt"

#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    FILE     *cross_file;
    MPI_Fint  itrue, ifalse;

    cross_file = fopen( CROSS_SPEC_OUTPUT, "a" );
    if ( cross_file == NULL ) {
        fprintf( stderr, "Can't open %s for appending!\n", CROSS_SPEC_OUTPUT );
        return 1;
    }

    /* Invoke the Fortran subroutine to get Fortran's TRUE/FALSE in C */
    flogical_( &itrue, &ifalse );
    fprintf( cross_file, "CROSS_FORTRAN2C_TRUE=%d\n", itrue );
    fprintf( cross_file, "CROSS_FORTRAN2C_FALSE=%d\n", ifalse );

    fclose( cross_file );
    return 0;
}
