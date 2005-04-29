/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"
#include "adioi.h"
#include "ad_ln_lnio.h"

void ADIOI_LN_Close(ADIO_File fd, int *error_code)
{
    int err;
    static char myname[] = "ADIOI_LN_CLOSE";
    int myrank, nprocs;

#ifdef PROFILE
    MPE_Log_event(9, 0, "start close");
#endif

    err = ADIOI_LNIO_Close(fd);

#ifdef PROFILE
    MPE_Log_event(10, 0, "end close");
#endif

    fd->fd_sys = -1;

    if (err == -1) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, MPI_ERR_IO,
					   "**io",
					   "**io %s", strerror(errno));
    }
    else *error_code = MPI_SUCCESS;


#ifdef JLEE_DEBUG
    MPI_Comm_size(fd->comm, &nprocs);
    MPI_Comm_rank(fd->comm, &myrank);
    FPRINTF(stdout, "[%d/%d] ADIOI_LN_Close called on %s\n", myrank, 
	    nprocs, fd->filename);
#endif
}
