/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Wait */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wait = PMPI_Wait
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wait  MPI_Wait
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wait as PMPI_Wait
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Wait PMPI_Wait

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Wait

/*@
   MPI_Wait - wait

   Arguments:
+  MPI_Request  *request - request
-  MPI_Status   *status - status

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Wait";
    MPID_Request * request_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAIT);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);
	    if (request != NULL)
	    {
		MPIR_ERRTEST_REQUEST(*request, mpi_errno);
	    }
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAIT);

    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL)
    {
	if (status != MPI_STATUS_IGNORE)
	{
	    status->MPI_SOURCE = MPI_ANY_SOURCE;
	    status->MPI_TAG = MPI_ANY_TAG;
	    status->MPI_ERROR = MPI_SUCCESS;
	    status->count = 0;
	    status->cancelled = FALSE;
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAIT);
	    return MPI_SUCCESS;
	}
    }
    
    /* Convert MPI request handle to a request object pointer */
    MPID_Request_get_ptr(*request, request_ptr);
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAIT);
                return MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    switch (request_ptr->kind)
    {
	case MPID_REQUEST_SEND:
	case MPID_REQUEST_RECV:
	{
	    *request = MPI_REQUEST_NULL;
	    MPIR_Wait(request_ptr);
	    break;
	}
	
	case MPID_PREQUEST_SEND:
	case MPID_PREQUEST_RECV:
	{
	    /* The device uses a partner request to track the actual
               communication.  If a partner request is not found, then the
               persistent request is inactive and an empty status is
               returned. */
	    if (request_ptr->partner_request != NULL)
	    {
		MPID_Request * req;

		req = request_ptr->partner_request;
		MPIR_Wait(req);
		request_ptr->partner_request = NULL;
		request_ptr = req;
	    }
	    else
	    {
		status->MPI_SOURCE = MPI_ANY_SOURCE;
		status->MPI_TAG = MPI_ANY_TAG;
		status->MPI_ERROR = MPI_SUCCESS;
		status->count = 0;
		status->cancelled = FALSE;
		MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAIT);
		return MPI_SUCCESS;
	    }
	    
	    break;
	}
	
	case MPID_UREQUEST:
	{
	    int rc;
	
	    *request = MPI_REQUEST_NULL;
	    
	    MPIR_Wait(request_ptr);
	    
	    rc = (request_ptr->query_fn)(
		request_ptr->grequest_extra_state, &request_ptr->status);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		mpi_errno = rc;
	    }
	    
	    rc = (request_ptr->free_fn)(request_ptr->grequest_extra_state);
	    if (mpi_errno == MPI_SUCCESS)
	    {
		mpi_errno = rc;
	    }

	    break;
	}
    }
    
    if (status != MPI_STATUS_IGNORE)
    {
	*status = request_ptr->status;
    }

    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = request_ptr->status.MPI_ERROR;
    }

    MPID_Request_release(request_ptr);

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAIT);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(request_ptr->comm, FCNAME, mpi_errno);
}
