/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

void ADIOI_XFS_WriteContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int err=-1;

    fd->fp_sys_posn = -1; /* set it to null, since we are using pwrite */

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
        if (fd->direct_write && !(((long) buf) % fd->d_mem) && 
            !(offset % fd->d_miniosz) && !(len % fd->d_miniosz) && 
            (len >= fd->d_miniosz) && (len <= fd->d_maxiosz)) {
	    err = pwrite(fd->fd_direct, buf, len, offset);
	    if ((err == -1) && (errno == EINVAL))
		err = pwrite(fd->fd_sys, buf, len, offset);
	}
	else err = pwrite(fd->fd_sys, buf, len, offset);
    }
    else {    /* write from curr. location of ind. file pointer */
        if (fd->direct_write && !(((long) buf) % fd->d_mem) && 
            !(offset % fd->d_miniosz) && !(len % fd->d_miniosz) && 
            (len >= fd->d_miniosz) && (len <= fd->d_maxiosz)) {
	    err = pwrite(fd->fd_direct, buf, len, fd->fp_ind);
	    if ((err == -1) && (errno == EINVAL))
		err = pwrite(fd->fd_sys, buf, len, fd->fp_ind);
	}
	else err = pwrite(fd->fd_sys, buf, len, fd->fp_ind);
	fd->fp_ind += err;
    }

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
}


void ADIOI_XFS_WriteStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    ADIOI_GEN_WriteStrided(fd, buf, count, datatype, file_ptr_type,
                        offset, status, error_code);
}
