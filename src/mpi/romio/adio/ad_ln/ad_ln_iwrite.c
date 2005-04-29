/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"

void ADIOI_LN_IwriteContig(ADIO_File fd, void *buf, int count, 
			   MPI_Datatype datatype, int file_ptr_type,
			   ADIO_Offset offset, ADIO_Request *request, 
			   int *error_code)  
{
    int len, typesize;
#ifndef ROMIO_HAVE_WORKING_LN_AIO
    ADIO_Status status;
#else
    /* nothing yet */
#endif
    static char myname[] = "ADIOI_LN_IWRITECONTIG";

    *request = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_WRITE;
    (*request)->fd = fd;
    (*request)->datatype = datatype;

    MPI_Type_size(datatype, &typesize);
    len = count * typesize;

#ifndef ROMIO_HAVE_WORKING_LN_AIO
    /* no support for nonblocking I/O. Use blocking I/O. */

    ADIO_WriteContig(fd, buf, len, MPI_BYTE, file_ptr_type, offset, 
		     &status, error_code);  
    (*request)->queued = 0;
# ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	(*request)->nbytes = len;
    }
# endif

    fd->fp_sys_posn = -1;

#else
    /* nothing yet */
#endif /* NO_AIO */

    fd->async_count++;
}

