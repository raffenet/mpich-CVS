/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_set_info - Sets new values for the hints associated with a file

Input Parameters:
. fh - file handle (handle)
. info - info object (handle)

.N fortran
@*/
int MPI_File_set_info(MPI_File fh, MPI_Info info)
{
    int error_code;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_set_info: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* set new info */
    ADIO_SetInfo(fh, info, &error_code);

    return error_code;
}
