/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

/*
 * pvfs2 will eventually have nonblocking io, but for now, just do it this way
 */
void ADIOI_PVFS2_IreadContig(ADIO_File fd, void *buf, int count, 
                MPI_Datatype datatype, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    ADIO_Status status;
    int len, typesize;

/* PVFS does not support nonblocking I/O. Therefore, use blocking I/O */

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_READ;
    (*request)->fd = fd;
    (*request)->queued = 0;
    (*request)->datatype = datatype;

    MPI_Type_size(datatype, &typesize);
    len = count * typesize;
    ADIOI_PVFS2_ReadContig(fd, buf, len, MPI_BYTE, file_ptr_type, offset, 
	    &status, error_code);  

#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	(*request)->nbytes = len;
    }
#endif
    fd->async_count++;
}

/* 
 * vim: ts=8 sts=4 sw=4 noexpandtab 
 */
