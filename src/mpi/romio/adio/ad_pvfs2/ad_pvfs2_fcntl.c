/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"
#include "adio_extern.h"
#include "ad_pvfs2_common.h"

void ADIOI_PVFS2_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct,
		       int *error_code)
{
    int ret;
    ADIOI_PVFS2_fs *pvfs_fs;
    PVFS_sysresp_getattr resp_getattr;
    int i, ntimes;
    ADIO_Offset curr_fsize, alloc_size, size, len, done;
    ADIO_Status status;
    char *buf;
    static char myname[] = "ADIOI_PVFS2_FCNTL";

    pvfs_fs = (ADIOI_PVFS2_fs*)fd->fs_ptr;

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
	ret = PVFS_sys_getattr(pvfs_fs->object_ref, PVFS_ATTR_SYS_SIZE, 
		&(pvfs_fs->credentials), &resp_getattr);
	if (ret != 0 ) {
	    /* --BEGIN ERROR HANDLING-- */
	    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					       MPIR_ERR_RECOVERABLE,
					       myname, __LINE__,
					       ADIOI_PVFS2_error_convert(ret),
					       "Error in PVFS_sys_getattr", 0);
	    /* --END ERROR HANDLING-- */
	}
	else {
	    *error_code = MPI_SUCCESS;
	}
	fcntl_struct->fsize = resp_getattr.attr.size;
	return;

    case ADIO_FCNTL_SET_DISKSPACE:
	/* TODO:this code is only slightly changed from every other 
	 * file system without a preallocate function.  Find some way to
	 * consolidate the routines */

	/* will be called by one process only */
	/* On file systems with no preallocation function, I have to 
           explicitly write 
           to allocate space. Since there could be holes in the file, 
           I need to read up to the current file size, write it back, 
           and then write beyond that depending on how much 
           preallocation is needed.
           read/write in sizes of no more than ADIOI_PREALLOC_BUFSZ */
	curr_fsize = fd->fp_ind;
	alloc_size = fcntl_struct->diskspace;

	size = ADIOI_MIN(curr_fsize, alloc_size);

	ntimes = (size + ADIOI_PREALLOC_BUFSZ - 1)/ADIOI_PREALLOC_BUFSZ;
	buf = (char *) ADIOI_Malloc(ADIOI_PREALLOC_BUFSZ);
	done = 0;

	for (i=0; i<ntimes; i++) {
	    len = ADIOI_MIN(size-done, ADIOI_PREALLOC_BUFSZ);
	    ADIO_ReadContig(fd, buf, len, MPI_BYTE, ADIO_EXPLICIT_OFFSET, done,
			    &status, error_code);
	    if (*error_code != MPI_SUCCESS) {
		/* --BEGIN ERROR HANDLING-- */
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						MPIR_ERR_RECOVERABLE,
						myname, __LINE__,
						MPI_ERR_IO, 
						"Error in ADIO_ReadContig", 0);
                return;  
		/* --END ERROR HANDLING-- */
	    }
	    ADIO_WriteContig(fd, buf, len, MPI_BYTE, ADIO_EXPLICIT_OFFSET, 
                             done, &status, error_code);
	    if (*error_code != MPI_SUCCESS) return;
	    done += len;
	}
	if (alloc_size > curr_fsize) {
	    memset(buf, 0, ADIOI_PREALLOC_BUFSZ); 
	    size = alloc_size - curr_fsize;
	    ntimes = (size + ADIOI_PREALLOC_BUFSZ - 1)/ADIOI_PREALLOC_BUFSZ;
	    for (i=0; i<ntimes; i++) {
		len = ADIOI_MIN(alloc_size-done, ADIOI_PREALLOC_BUFSZ);
		ADIO_WriteContig(fd, buf, len, MPI_BYTE, ADIO_EXPLICIT_OFFSET, 
				 done, &status, error_code);
		if (*error_code != MPI_SUCCESS) return;
		done += len;  
	    }
	}
	ADIOI_Free(buf);
	*error_code = MPI_SUCCESS;
	break;

    /* --BEGIN ERROR HANDLING-- */
    case ADIO_FCNTL_SET_ATOMICITY:
    default:
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_ARG,
					   "Unknown flag passed to ADIOI_PVFS2_FCNTL", 0);
    /* --END ERROR HANDLING-- */
    }
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
