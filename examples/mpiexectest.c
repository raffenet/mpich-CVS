/* simple test for multiple executables */
#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#define MAX_DIRNAME_SIZE 256 

int main( int argc, char *argv[], char *envp[] )
{
    int  i, myid, numprocs;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    char curr_wd[MAX_DIRNAME_SIZE];

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
    MPI_Comm_rank( MPI_COMM_WORLD, &myid );
    MPI_Get_processor_name( processor_name, &namelen );

    fprintf( stdout, "[%d] Process %d of %d (%s) is on %s\n",
	     myid, myid, numprocs, argv[0], processor_name );
    fflush( stdout );

    for ( i = 1; i < argc; i++ ) {
	fprintf( stdout, "[%d] argv[%d]=\"%s\"\n", myid, i, argv[i] ); 
	fflush( stdout );
    }

    getcwd( curr_wd, MAX_DIRNAME_SIZE ); 
    fprintf( stdout, "[%d] current working directory=%s\n", i, curr_wd );

#ifdef PRINTENV
    /* may produce lots of output, but here if you need it */
    for ( i = 0; envp[i]; i++ )
	fprintf( stdout, "[%d] envp[%d]=%s\n", myid, i, envp[i] );
#endif

    MPI_Finalize( );
}
