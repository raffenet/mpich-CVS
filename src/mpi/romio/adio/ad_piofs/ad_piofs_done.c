/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_piofs.h"

int ADIOI_PIOFS_ReadDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{

    if (*request != ADIO_REQUEST_NULL) {
	(*request)->fd->async_count--;
	ADIOI_Free_request((ADIOI_Req_node *) (*request));
	*request = ADIO_REQUEST_NULL;
    }

    *error_code = MPI_SUCCESS;
    return 1;

/* status to be filled */

}


int ADIOI_PIOFS_WriteDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    ADIOI_PIOFS_ReadDone(request, status, error_code);
} 
