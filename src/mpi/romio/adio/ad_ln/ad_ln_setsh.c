/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"
#include "adioi.h"

void ADIOI_LN_Set_shared_fp(ADIO_File fd, ADIO_Offset offset, 
			    int *error_code)
{
    int myrank, nprocs;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_LN_Set_shared_fp called on %s\n", 
	    myrank, nprocs, fd->filename);
}
