/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_get_position_shared - Returns the current position of the 
               shared file pointer in etype units relative to the current view

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. offset - offset of shared file pointer (nonnegative integer)

.N fortran
@*/
int MPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset)
{
    int error_code;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_position_shared: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
        printf("MPI_File_get_position_shared: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((fh->file_system == ADIO_PIOFS) || (fh->file_system == ADIO_PVFS)) {
	printf("MPI_File_get_position_shared: Shared file pointer not supported on PIOFS and PVFS\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIO_Get_shared_fp(fh, 0, offset, &error_code);
    return error_code;
}
