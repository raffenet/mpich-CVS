/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"
#include "adio_extern.h"
/* #ifdef MPISGI
#include "mpisgi2.h"
#endif */

void ADIOI_NFS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct, int *error_code)
{
    int i, ntimes, len;
    ADIO_Offset curr_fsize, alloc_size, size, done;
    ADIO_Status status;
    char *buf;
    static char myname[] = "ADIOI_NFS_FCNTL";

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
	ADIOI_READ_LOCK(fd, 0, SEEK_SET, 1);
	fcntl_struct->fsize = lseek(fd->fd_sys, 0, SEEK_END);
	ADIOI_UNLOCK(fd, 0, SEEK_SET, 1);
	if (fd->fp_sys_posn != -1) {
	    lseek(fd->fd_sys, fd->fp_sys_posn, SEEK_SET);
	}
	if (fcntl_struct->fsize == -1) {
	    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					       MPIR_ERR_RECOVERABLE, myname,
					       __LINE__, MPI_ERR_IO, "**io",
					       "**io %s", strerror(errno));
	}
	else *error_code = MPI_SUCCESS;
	break;

    case ADIO_FCNTL_SET_DISKSPACE:
	/* will be called by one process only */
	/* On file systems with no preallocation function, I have to 
           explicitly write 
           to allocate space. Since there could be holes in the file, 
           I need to read up to the current file size, write it back, 
           and then write beyond that depending on how much 
           preallocation is needed.
           read/write in sizes of no more than ADIOI_PREALLOC_BUFSZ */

	curr_fsize = lseek(fd->fd_sys, 0, SEEK_END);
	alloc_size = fcntl_struct->diskspace;

	size = ADIOI_MIN(curr_fsize, alloc_size);
	
	ntimes = (int) ((size + ADIOI_PREALLOC_BUFSZ - 1)/ADIOI_PREALLOC_BUFSZ);
	buf = (char *) ADIOI_Malloc(ADIOI_PREALLOC_BUFSZ);
	done = 0;

	for (i=0; i<ntimes; i++) {
	    len = (int) (ADIOI_MIN(size-done, ADIOI_PREALLOC_BUFSZ));
	    ADIO_ReadContig(fd, buf, len, MPI_BYTE, ADIO_EXPLICIT_OFFSET, done,
			    &status, error_code);
	    if (*error_code != MPI_SUCCESS) {
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						   MPIR_ERR_RECOVERABLE,
						   myname, __LINE__,
						   MPI_ERR_IO, "**io",
						   "**io %s", strerror(errno));
		return;  
	    }
	    ADIO_WriteContig(fd, buf, len, MPI_BYTE, ADIO_EXPLICIT_OFFSET,
			     done, &status, error_code);
	    if (*error_code != MPI_SUCCESS) return;
	    done += len;
	}

	if (alloc_size > curr_fsize) {
	    memset(buf, 0, ADIOI_PREALLOC_BUFSZ); 
	    size = alloc_size - curr_fsize;
	    ntimes = (int) ((size + ADIOI_PREALLOC_BUFSZ - 1)/ADIOI_PREALLOC_BUFSZ);
	    for (i=0; i<ntimes; i++) {
		len = (int) (ADIOI_MIN(alloc_size-done, ADIOI_PREALLOC_BUFSZ));
		ADIO_WriteContig(fd, buf, len, MPI_BYTE, ADIO_EXPLICIT_OFFSET, 
				 done, &status, error_code);
		if (*error_code != MPI_SUCCESS) return;
		done += len;  
	    }
	}
	ADIOI_Free(buf);
	if (fd->fp_sys_posn != -1) 
	    lseek(fd->fd_sys, fd->fp_sys_posn, SEEK_SET);
	*error_code = MPI_SUCCESS;
	break;

    case ADIO_FCNTL_SET_ATOMICITY:
	fd->atomicity = (fcntl_struct->atomicity == 0) ? 0 : 1;
	*error_code = MPI_SUCCESS;
	break;

    default:
	/* --BEGIN ERROR HANDLING-- */
	*error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					   myname, __LINE__,
					   MPI_ERR_INTERN, "**io",
					   "**io %s",
					   "Unknown flag passed to ADIOI_NFS_Fcntl");
	return;
	/* --END ERROR HANDLING-- */
    }
}
