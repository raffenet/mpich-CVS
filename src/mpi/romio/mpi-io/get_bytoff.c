/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_get_byte_offset - Returns the absolute byte position in 
                the file corresponding to "offset" etypes relative to
                the current view

Input Parameters:
. fh - file handle (handle)
. offset - offset (nonnegative integer)

Output Parameters:
. disp - absolute byte position of offset (nonnegative integer)

.N fortran
@*/
int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset *disp)
{
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_byte_offset: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (offset < 0) {
        printf("MPI_File_get_byte_offset: Invalid offset argument\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
	printf("MPI_File_get_byte_offset: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIOI_Get_byte_offset(fh, offset, disp);
    return MPI_SUCCESS;
}
