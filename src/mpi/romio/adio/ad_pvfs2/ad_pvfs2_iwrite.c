/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_pvfs2.h"

/*
 * pvfs2 will someday support nonblocking io, but until then we have to do
 * blocking io.
 */

void ADIOI_PVFS2_IwriteContig(ADIO_File fd, void *buf, int count, 
                MPI_Datatype datatype, int file_ptr_type,
                ADIO_Offset offset, ADIO_Request *request, int *error_code)  
{
    ADIO_Status status;
    int len, typesize;

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->queued = 0;
    (*request)->datatype = datatype;

    MPI_Type_size(datatype, &typesize);
    len = count * typesize;
    ADIOI_PVFS2_WriteContig(fd, buf, len, MPI_BYTE, file_ptr_type, offset, &status,
		    error_code);  

#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	(*request)->nbytes = len;
    }
#endif
    fd->async_count++;
}
