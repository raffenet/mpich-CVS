/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_get_group - Returns the group of processes that 
                         opened the file

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. group - group that opened the file (handle)

.N fortran
@*/
int MPI_File_get_group(MPI_File fh, MPI_Group *group)
{
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_group: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return MPI_Comm_group(fh->comm, group);
}
