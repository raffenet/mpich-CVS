/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

void ADIOI_XFS_Close(ADIO_File fd, int *error_code)
{
    int err, err1;

    err = close(fd->fd_sys);
    err1 = close(fd->fd_direct);

    *error_code = ((err == 0) && (err1 == 0)) ? MPI_SUCCESS : MPI_ERR_UNKNOWN;
}
