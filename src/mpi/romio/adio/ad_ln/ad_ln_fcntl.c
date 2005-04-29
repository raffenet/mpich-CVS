/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"
#include "adioi.h"
#include "adio_extern.h"

void ADIOI_LN_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, 
		    int *error_code)
{
    struct lnio_handle_t *handle = (struct lnio_handle_t *)fd->fs_ptr;
    static char myname[] = "ADIOI_LN_FCNTL";

    *error_code = MPI_SUCCESS;

#ifdef JLEE_DEBUG
    int myrank, nprocs;

    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_LN_Fcntl called on %s\n", 
	    myrank, nprocs, fd->filename);
#endif 

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
	fcntl_struct->fsize = handle->ex->logical_length;
	*error_code = MPI_SUCCESS;
	break;

    case ADIO_FCNTL_SET_DISKSPACE:
	ADIOI_GEN_Prealloc(fd, fcntl_struct->diskspace, error_code);
	*error_code = MPI_SUCCESS;
	break;

    case ADIO_FCNTL_SET_ATOMICITY:
	fd->atomicity = (fcntl_struct->atomicity == 0) ? 0 : 1;
	*error_code = MPI_SUCCESS;
	break;

    default:
	/* --BEGIN ERROR HANDLING-- */
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, 
					   MPI_ERR_ARG,
					   "**flag", "**flag %d", flag);
	return;
	/* --END ERROR HANDLING-- */
    }
}
