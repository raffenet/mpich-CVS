#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SPAWNED 32

int main( int argc, char * argv[] )
{
    int rank;
    int size;
    int rc, maxprocs = 1;
    char *args[5];
    MPI_Comm newintercomm;
    int errcodes[MAX_SPAWNED];
    char executable[128];

    MPI_Init( 0, 0 );
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf( "spawner %d of %d is alive\n", rank, size ); 

    if ( argc > 1 ) {
	maxprocs = atoi( argv[1] );
	if ( maxprocs > MAX_SPAWNED ) {
	    printf( "must spawn fewer than %d spawnees\n", MAX_SPAWNED );
	    exit( -1 );
	}
    }
    strncpy( executable, "/sandbox/lusk/holdit/mpich2-mpd-aug28/examples/spawnee", 128 );
    args[0] = "argument_1";
    args[1] = (char *) 0;
    rc = MPI_Comm_spawn( executable, args, maxprocs, MPI_INFO_NULL, 0, MPI_COMM_WORLD,
			 &newintercomm, errcodes );
    if ( rc == MPI_SUCCESS )
	printf( "spawner %d succeeded\n", rank );
    else
	printf( "spawner %d returned %d\n", rank, rc );

    MPI_Finalize();
    return 0;
}
    
