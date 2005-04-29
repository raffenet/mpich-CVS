/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"
#include "ad_ln_lnio.h"

void ADIOI_LN_IreadContig(ADIO_File fd, void *buf, int count, 
			  MPI_Datatype datatype, int file_ptr_type,
			  ADIO_Offset offset, ADIO_Request *request, 
			  int *error_code)  
{
    int len, typesize;
#ifndef ROMIO_HAVE_WORKING_LN_AIO
    ADIO_Status status;
#else
    /* no async implementation of LN I/O yet */
#endif
    static char myname[] = "ADIOI_LN_IREADCONTIG";

    (*request) = ADIOI_Malloc_request();
    (*request)->optype = ADIOI_READ;
    (*request)->fd = fd;
    (*request)->datatype = datatype;

    MPI_Type_size(datatype, &typesize);
    len = count * typesize;

#ifndef ROMIO_HAVE_WORKING_LN_AIO
    /* HP, FreeBSD, Linux */
    /* no support for nonblocking I/O. Use blocking I/O. */

    ADIO_ReadContig(fd, buf, len, MPI_BYTE, file_ptr_type, offset, 
		    &status, error_code);  
    (*request)->queued = 0;
#ifdef HAVE_STATUS_SET_BYTES
    if (*error_code == MPI_SUCCESS) {
	MPI_Get_elements(&status, MPI_BYTE, &len);
	(*request)->nbytes = len;
    }
#endif

    fd->fp_sys_posn = -1;

#else
    /* we don't have asynch. LN I/O yet */
#endif  /* NO_AIO */

    fd->async_count++;
}
