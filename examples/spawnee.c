#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int rank;
    int size;
    char *kvsname;

    MPI_Init( 0, 0 );
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    kvsname = (char *) malloc(PMI_KVS_Get_name_length_max( ) );
    PMI_KVS_Get_my_name( kvsname );
    printf( "Hello world from spawned process %d of %d, kvsname=%s\n", rank, size, kvsname );
    MPI_Finalize();
    return 0;
}
