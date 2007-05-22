#include "cross_conf.h"

#define CROSS_SPEC_OUTPUT  "cross_spec.txt"

#include <stdio.h>
#include "mpi.h"

static char* is_runtime_bigendian( void )
{
    union {
        long ll;
        char cc[sizeof(long)];
    } uu;
    uu.ll = 1;
    if ( uu.cc[sizeof(long) - 1] == 1 )
        return "true";
    else
        return "false";
}

int main( int argc, char *argv[] )
{
    FILE     *cross_file;
    MPI_Fint  itrue, ifalse;

    cross_file = fopen( CROSS_SPEC_OUTPUT, "w" );
    if ( cross_file == NULL ) {
        fprintf( stderr, "Can't open %s for writing!\n", CROSS_SPEC_OUTPUT );
        return 1;
    }

    fprintf( cross_file, "CROSS_SIZEOF_CHAR=%d\n", sizeof(char) );
    fprintf( cross_file, "CROSS_SIZEOF_SHORT=%d\n", sizeof(short) );
    fprintf( cross_file, "CROSS_SIZEOF_INT=%d\n", sizeof(int) );
    fprintf( cross_file, "CROSS_SIZEOF_LONG=%d\n", sizeof(long) );
    fprintf( cross_file, "CROSS_SIZEOF_LONG_LONG=%d\n", sizeof(long long) );
    fprintf( cross_file, "CROSS_SIZEOF_VOIDP=%d\n", sizeof(void*) );
    fprintf( cross_file, "CROSS_BIGENDIAN=%s\n", is_runtime_bigendian() );
    fprintf( cross_file, "CROSS_MPI_STATUS_SIZE=%d\n",
                          sizeof(MPI_Status)/sizeof(MPI_Fint) );

    fclose( cross_file );
    return 0;
}
