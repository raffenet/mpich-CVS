#include "mpi.h"

#include <stdio.h>

int main( int argc, char *argv[] )
{
    MPI_Datatype newtype;

    MPI_Init( 0, 0 );

    MPI_Type_hvector( 0, 1, 10, MPI_DOUBLE, &newtype );
    MPI_Type_commit( &newtype );

    MPI_Finalize();
}
