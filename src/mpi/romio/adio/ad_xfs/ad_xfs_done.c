/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_xfs.h"

int ADIOI_XFS_ReadDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    int err, nbytes, done=0;

    if (*request == ADIO_REQUEST_NULL) {
	*error_code = MPI_SUCCESS;
	return 1;
    }

    if ((*request)->next != ADIO_REQUEST_NULL) {
	done = ADIOI_XFS_ReadDone(&((*request)->next), status, error_code);
    /* currently passing status and error_code here, but something else
       needs to be done to get the status and error info correctly */
	if (!done) {
	   *error_code = MPI_SUCCESS;
	   return done;
	}
    }

    if ((*request)->queued) {
	err = aio_error64((const aiocb64_t *) (*request)->handle);
	if (err == EINPROGRESS) {
	    done = 0;
	    *error_code = MPI_SUCCESS;
	}
	else {
	    nbytes = aio_return64((aiocb64_t *) (*request)->handle); 
	    /* also dequeues the request*/ 
	    /*  if (err) printf("error in testing completion of nonblocking I/O\n");*/
	    done = 1;
	    *error_code = (err == -1) ? MPI_ERR_UNKNOWN : MPI_SUCCESS;
	    /* status to be filled */
	}
    }
    else {
	done = 1;
	*error_code = MPI_SUCCESS;
    }

    if (done) {
	/* if request is still queued in the system, it is also there
           on ADIOI_Async_list. Delete it from there. */
	if ((*request)->queued) ADIOI_Del_req_from_list(request);

	(*request)->fd->async_count--;
	if ((*request)->handle) ADIOI_Free((*request)->handle);
	ADIOI_Free_request((ADIOI_Req_node *) (*request));
	*request = ADIO_REQUEST_NULL;
	/* status to be filled */
    }
    return done;
}


int ADIOI_XFS_WriteDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    return ADIOI_XFS_ReadDone(request, status, error_code);
} 
