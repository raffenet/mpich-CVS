/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ufs.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void ADIOI_UFS_Resize(ADIO_File fd, ADIO_Offset size, int *error_code)
{
    int err;
    static char myname[] = "ADIOI_UFS_RESIZE";
    
    err = ftruncate(fd->fd_sys, size);
    if (err == -1) {
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					   myname, __LINE__, MPI_ERR_IO, "**io",
					   "**io %s", strerror(errno));
    }
    else *error_code = MPI_SUCCESS;
}
