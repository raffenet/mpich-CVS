/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_testfs.h"
#include "adioi.h"

ADIO_Offset ADIOI_TESTFS_SeekIndividual(ADIO_File fd, ADIO_Offset offset, 
					int whence, int *error_code)
{
    int myrank, nprocs;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_TESTFS_SeekIndividual called on %s\n", 
	    myrank, nprocs, fd->filename);

    return (ADIO_Offset) 0;
}
