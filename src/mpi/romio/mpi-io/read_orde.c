/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_read_ordered_end - Complete a split collective read using shared file pointer

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int MPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status)
{

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_read_ordered_end: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (buf < (void *) 0) {
        printf("MPI_File_read_ordered_end: buf is not a valid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (!(fh->split_coll_count)) {
        printf("MPI_File_read_ordered_end: Does not match a previous MPI_File_read_ordered_begin\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fh->split_coll_count = 0;

    return MPI_SUCCESS;
}
