/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ufs.h"

void ADIOI_UFS_WriteContig(ADIO_File fd, void *buf, int len, int file_ptr_type,
		     ADIO_Offset offset, ADIO_Status *status, int *error_code)
{
    int err=-1;

    if ((fd->iomode == M_ASYNC) || (fd->iomode == M_UNIX)) {
	if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
	    if (fd->fp_sys_posn != offset)
		lseek(fd->fd_sys, offset, SEEK_SET);
	    err = write(fd->fd_sys, buf, len);
	    fd->fp_sys_posn = offset + err;
         /* individual file pointer not updated */        
        }
	else { /* write from curr. location of ind. file pointer */
	    if (fd->fp_sys_posn != fd->fp_ind)
		lseek(fd->fd_sys, fd->fp_ind, SEEK_SET);
	    err = write(fd->fd_sys, buf, len);
	    fd->fp_ind += err;
	    fd->fp_sys_posn = fd->fp_ind;
        }
    }
    else fd->fp_sys_posn = -1;    /* set it to null */

    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
}



void ADIOI_UFS_WriteStrided(ADIO_File fd, void *buf, int count,
                       MPI_Datatype datatype, int file_ptr_type,
                       ADIO_Offset offset, ADIO_Status *status, int
                       *error_code)
{
    ADIOI_GEN_WriteStrided(fd, buf, count, datatype, file_ptr_type,
                        offset, status, error_code);
}
