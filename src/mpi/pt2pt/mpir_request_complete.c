/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

int MPIR_Request_complete(MPI_Request * request, MPID_Request * request_ptr,
			  MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    
    switch(request_ptr->kind)
    {
	case MPID_REQUEST_SEND:
	case MPID_REQUEST_RECV:
	{
	    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS)
	    {
		mpi_errno = request_ptr->status.MPI_ERROR;
	    }

	    if (status != MPI_STATUS_IGNORE)
	    {
		*status = request_ptr->status;
	    }
	    
	    *request = MPI_REQUEST_NULL;
	    MPID_Request_release(request_ptr);
	    break;
	}
			
	case MPID_PREQUEST_SEND:
	case MPID_PREQUEST_RECV:
	{
	    if (request_ptr->partner_request != NULL)
	    {
		MPID_Request * prequest_ptr;

		prequest_ptr = request_ptr->partner_request;
		request_ptr->partner_request = NULL;
		
		if (prequest_ptr->status.MPI_ERROR != MPI_SUCCESS)
		{
		    mpi_errno = prequest_ptr->status.MPI_ERROR;
		}
			    
		if (status != MPI_STATUS_IGNORE)
		{
		    *status = prequest_ptr->status;
		}
	    
		MPID_Request_release(prequest_ptr);
	    }
	    else
	    {
		if (status != MPI_STATUS_IGNORE)
		{
		    status->MPI_SOURCE = MPI_ANY_SOURCE;
		    status->MPI_TAG = MPI_ANY_TAG;
		    status->MPI_ERROR = MPI_SUCCESS;
		    status->count = 0;
		    status->cancelled = FALSE;
		}
	    }
	    
	    break;
	}

	case MPID_UREQUEST:
	{
	    int rc;
	    
	    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS)
	    {
		mpi_errno = request_ptr->status.MPI_ERROR;
	    }
			    
	    rc = (request_ptr->query_fn)(request_ptr->grequest_extra_state,
					 &request_ptr->status);
	    if (request_ptr->status.MPI_ERROR == MPI_SUCCESS)
	    {
		request_ptr->status.MPI_ERROR = rc;
		mpi_errno = rc;
	    }
	    
	    rc = (request_ptr->free_fn)(request_ptr->grequest_extra_state);
	    if (request_ptr->status.MPI_ERROR == MPI_SUCCESS)
	    {
		request_ptr->status.MPI_ERROR = rc;
		mpi_errno = rc;
	    }
	    
	    if (status != MPI_STATUS_IGNORE)
	    {
		*status = request_ptr->status;
	    }
	    
	    MPID_Request_release(request_ptr);
	    *request = MPI_REQUEST_NULL;
	    break;
	}
    }

    return mpi_errno;
}
