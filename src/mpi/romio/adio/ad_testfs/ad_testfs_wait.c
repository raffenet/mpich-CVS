/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_testfs.h"
#include "adioi.h"

void ADIOI_TESTFS_ReadComplete(ADIO_Request *request, ADIO_Status *status, int
			       *error_code)
{
    int myrank, nprocs;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size((*request)->fd->comm, &nprocs);
    MPI_Comm_rank((*request)->fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_TESTFS_ReadComplete called on %s\n", 
	    myrank, nprocs, (*request)->fd->filename);
}

void ADIOI_TESTFS_WriteComplete(ADIO_Request *request, ADIO_Status *status, int
				*error_code)
{
    int myrank, nprocs;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size((*request)->fd->comm, &nprocs);
    MPI_Comm_rank((*request)->fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_TESTFS_WriteComplete called on %s\n", 
	    myrank, nprocs, (*request)->fd->filename);
}
