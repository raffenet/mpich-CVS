/*
  Check that MPI_xxxx_c2f, applied to the same object several times,
  yields the same handle.  We do this because when MPI handles in 
  C are a different length than those in Fortran, care needs to 
  be exercised to ensure that the mapping from one to another is unique.
  (Test added to test a potential problem in ROMIO for handling MPI_File
  on 64-bit systems)
*/
#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] )
{
    MPI_Fint handleA, handleB;
    int      rc;
    int      errs = 0;
    MPI_File cFile;

    MPI_Init( &argc, &argv );

    /* File */
    rc = MPI_File_open( MPI_COMM_WORLD, "temp", 
		   MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE | MPI_MODE_CREATE, 
		   MPI_INFO_NULL, &cFile );
    if (rc) {
	errs++;
	printf( "Unable to open file \"temp\"\n" );
    }
    else {
	handleA = MPI_File_c2f( cFile );
	handleB = MPI_File_c2f( cFile );
	if (handleA != handleB) {
	    errs++;
	    printf( "MPI_File_c2f does not give the same handle twice on the same MPI_File\n" );
	}
    }
    MPI_File_close( &cFile );

    
    MPI_Finalize();
    
    return 0;
}
