/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Test */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Test = PMPI_Test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Test  MPI_Test
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Test as PMPI_Test
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Test PMPI_Test

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Test

/*@
    MPI_Test  - Tests for the completion of a send or receive

Input Parameter:
. request - communication request (handle) 

Output Parameter:
+ flag - true if operation completed (logical) 
- status - status object (Status).  May be 'MPI_STATUS_NULL'.

.N waitstatus

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Test";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TEST);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag,"flag",mpi_errno);
	    if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TEST);

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
	    *flag = TRUE;
	}
	
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST);
	return MPI_SUCCESS;
    }
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( *request, request_ptr );
    
    /* Validate objects if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate request_ptr */
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
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
	    MPIR_Test(request_ptr, *flag);
	    if (*flag)
	    {
		*request = MPI_REQUEST_NULL;

		if (status != MPI_STATUS_IGNORE)
		{
		    *status = request_ptr->status;
		}
		mpi_errno = request_ptr->status.MPI_ERROR;
		MPID_Request_release(request_ptr);
	    }
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
		MPIR_Test(req, *flag);
		if (*flag)
		{
		    request_ptr->partner_request = NULL;
		    
		    if (status != MPI_STATUS_IGNORE)
		    {
			*status = req->status;
		    }
		    mpi_errno = req->status.MPI_ERROR;
		    MPID_Request_release(req);
		}
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
		    *flag = TRUE;
		}
	    }
	    
	    break;
	}
	
	case MPID_UREQUEST:
	{
	    int rc;
	
	    MPIR_Test(request_ptr, *flag);
	    if (*flag)
	    {
		*request = MPI_REQUEST_NULL;
		rc = (request_ptr->query_fn)(
		    request_ptr->grequest_extra_state, &request_ptr->status);
		if (rc == MPI_SUCCESS)
		{
		    if (status != MPI_STATUS_IGNORE)
		    {
			*status = request_ptr->status;
		    }
		    mpi_errno = request_ptr->status.MPI_ERROR;
		}
		else
		{
		    if (mpi_errno == MPI_SUCCESS)
		    {
			mpi_errno = rc;
		    }
		}
		
		rc = (request_ptr->free_fn)(request_ptr->grequest_extra_state);
		if (mpi_errno == MPI_SUCCESS)
		{
		    mpi_errno = rc;
		}

		MPID_Request_release(request_ptr);
	    }
	    
	    break;
	}
    }

    /* ... end of body of routine ... */

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(request_ptr->comm, FCNAME, mpi_errno);
}
