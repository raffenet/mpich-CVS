#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    MPI_Init( 0, 0 );
    printf( "Hello world\n" );
    MPI_Finalize();
    return 0;
}
