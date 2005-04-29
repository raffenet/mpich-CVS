/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_ln.h"

void ADIOI_LN_ReadComplete(ADIO_Request *request, ADIO_Status *status,
			   int *error_code)  
{
#ifdef ROMIO_HAVE_WORKING_LN_AIO
    /* nothing yet */
#endif
    static char myname[] = "ADIOI_LN_READCOMPLETE";
    
    if (*request == ADIO_REQUEST_NULL) {
	*error_code = MPI_SUCCESS;
	return;
    }
}


void ADIOI_LN_WriteComplete(ADIO_Request *request, ADIO_Status *status, 
			    int *error_code)  
{
    ADIOI_LN_ReadComplete(request, status, error_code);
}
