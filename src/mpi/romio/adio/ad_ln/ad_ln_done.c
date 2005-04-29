/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"

int ADIOI_LN_ReadDone(ADIO_Request *request, ADIO_Status *status,
		      int *error_code)  
{
#ifdef ROMIO_HAVE_WORKING_LN_AIO
    /* nothing yet */
#endif

    static char myname[] = "ADIOI_LN_READDONE";

    if (*request == ADIO_REQUEST_NULL) {
	*error_code = MPI_SUCCESS;
	return 1;
    }

#ifndef ROMIO_HAVE_WORKING_LN_AIO
#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, (*request)->datatype, (*request)->nbytes);
#endif
    (*request)->fd->async_count--;
    ADIOI_Free_request((ADIOI_Req_node *) (*request));
    *request = ADIO_REQUEST_NULL;
    *error_code = MPI_SUCCESS;
    return 1;
#endif    
}


int ADIOI_LN_WriteDone(ADIO_Request *request, ADIO_Status *status,
		       int *error_code)
{
    return ADIOI_LN_ReadDone(request, status, error_code);
} 
