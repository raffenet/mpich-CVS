/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

void ADIOI_XFS_ReadContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int err=-1;
#ifndef __PRINT_ERR_MSG
    static char myname[] = "ADIOI_XFS_READCONTIG";
#endif

    fd->fp_sys_posn = -1; /* set it to null, since we are using pread */

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
        if (fd->direct_read && !(((long) buf) % fd->d_mem) && 
            !(offset % fd->d_miniosz) && !(len % fd->d_miniosz) && 
            (len >= fd->d_miniosz) && (len <= fd->d_maxiosz)) {
	    err = pread(fd->fd_direct, buf, len, offset);
	    if ((err == -1) && (errno == EINVAL))
		err = pread(fd->fd_sys, buf, len, offset);
	}
	else err = pread(fd->fd_sys, buf, len, offset);
    }
    else {    /* read from curr. location of ind. file pointer */
        if (fd->direct_read && !(((long) buf) % fd->d_mem) && 
            !(offset % fd->d_miniosz) && !(len % fd->d_miniosz) && 
            (len >= fd->d_miniosz) && (len <= fd->d_maxiosz)) {
	    err = pread(fd->fd_direct, buf, len, fd->fp_ind);
	    if ((err == -1) && (errno == EINVAL))
		err = pread(fd->fd_sys, buf, len, fd->fp_ind);
	}
	else err = pread(fd->fd_sys, buf, len, fd->fp_ind);
	fd->fp_ind += err;
    }

#ifdef __PRINT_ERR_MSG
    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
#else
    if (err == -1) {
	*error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ADIO_ERROR,
			      myname, "I/O Error", "%s", strerror(errno));
	ADIOI_Error(fd, *error_code, myname);	    
    }
    else *error_code = MPI_SUCCESS;
#endif
}



void ADIOI_XFS_ReadStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    ADIOI_GEN_ReadStrided(fd, buf, count, datatype, file_ptr_type,
                        offset, status, error_code);
}
