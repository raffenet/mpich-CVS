/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_nfs.h"

int ADIOI_NFS_ReadDone(ADIO_Request *request, ADIO_Status *status,
		       int *error_code)
{
    static char myname[] = "ADIOI_NFS_READDONE";
#ifndef NO_AIO
    int done=0;
    int err;
#ifdef AIO_HANDLE_IN_AIOCB
    struct aiocb *tmp1;
#endif
#endif

    if (*request == ADIO_REQUEST_NULL) {
	*error_code = MPI_SUCCESS;
	return 1;
    }

#ifdef NO_AIO
/* HP, FreeBSD, Linux */
#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, (*request)->datatype, (*request)->nbytes);
#endif
    (*request)->fd->async_count--;
    ADIOI_Free_request((ADIOI_Req_node *) (*request));
    *request = ADIO_REQUEST_NULL;
    *error_code = MPI_SUCCESS;
    return 1;
#endif    

#ifdef AIO_HANDLE_IN_AIOCB
/* IBM */
    if ((*request)->queued) {
	tmp1 = (struct aiocb *) (*request)->handle;
	errno = aio_error(tmp1->aio_handle);
	if (errno == EINPROG) {
	    done = 0;
	    *error_code = MPI_SUCCESS;
	}
	else {
	    err = aio_return(tmp1->aio_handle);
	    (*request)->nbytes = err;
	    errno = aio_error(tmp1->aio_handle);
	
	    done = 1;

	    if (err == -1) {
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						   MPIR_ERR_RECOVERABLE,
						   myname, __LINE__,
						   MPI_ERR_IO, "**io",
						   "**io %s", strerror(errno));
	    }
	    else *error_code = MPI_SUCCESS;
	}
    }
    else {
	done = 1;
	*error_code = MPI_SUCCESS;
    }
#ifdef HAVE_STATUS_SET_BYTES
    if (done && ((*request)->nbytes != -1))
	MPIR_Status_set_bytes(status, (*request)->datatype, (*request)->nbytes);
#endif

#elif !defined(NO_AIO)
/* DEC, SGI IRIX 5 and 6 */
    if ((*request)->queued) {
	errno = aio_error((const struct aiocb *) (*request)->handle);
	if (errno == EINPROGRESS) {
	    done = 0;
	    *error_code = MPI_SUCCESS;
	}
	else {
	    err = aio_return((struct aiocb *) (*request)->handle); 
	    (*request)->nbytes = err;
	    errno = aio_error((struct aiocb *) (*request)->handle);

	    done = 1;

	    if (err == -1) {
		*error_code = MPIO_Err_create_code(MPI_SUCCESS,
						   MPIR_ERR_RECOVERABLE,
						   myname, __LINE__,
						   MPI_ERR_IO, "**io",
						   "**io %s", strerror(errno));
	    }
	    else *error_code = MPI_SUCCESS;
	}
    }
    else {
	done = 1;
	*error_code = MPI_SUCCESS;
    }
#ifdef HAVE_STATUS_SET_BYTES
    if (done && ((*request)->nbytes != -1))
	MPIR_Status_set_bytes(status, (*request)->datatype, (*request)->nbytes);
#endif

#endif

#ifndef NO_AIO
    if (done) {
	/* if request is still queued in the system, it is also there
           on ADIOI_Async_list. Delete it from there. */
	if ((*request)->queued) ADIOI_Del_req_from_list(request);

	(*request)->fd->async_count--;
	if ((*request)->handle) ADIOI_Free((*request)->handle);
	ADIOI_Free_request((ADIOI_Req_node *) (*request));
	*request = ADIO_REQUEST_NULL;
    }
    return done;
#endif

}


int ADIOI_NFS_WriteDone(ADIO_Request *request, ADIO_Status *status, int *error_code)  
{
    return ADIOI_NFS_ReadDone(request, status, error_code);
} 
