/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_piofs.h"
#ifdef PROFILE
#include "mpe.h"
#endif

void ADIOI_PIOFS_Close(ADIO_File fd, int *error_code)
{
    int err;
    static char myname[] = "ADIOI_PIOFS_CLOSE";

#ifdef PROFILE
    MPE_Log_event(9, 0, "start close");
#endif
    err = close(fd->fd_sys);
#ifdef PROFILE
    MPE_Log_event(10, 0, "end close");
#endif
    fd->fd_sys = -1;
    if (err == -1) {
	*error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, MPI_ERR_IO,
					   "**io",
					   "**io %s", strerror(errno));
    }
    else *error_code = MPI_SUCCESS;
}
