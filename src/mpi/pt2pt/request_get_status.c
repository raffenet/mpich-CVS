/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Request_get_status */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Request_get_status = PMPI_Request_get_status
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Request_get_status  MPI_Request_get_status
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Request_get_status as PMPI_Request_get_status
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Request_get_status PMPI_Request_get_status

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Request_get_status

/*@
   MPI_Request_get_status - Nondestructive test for the completion of a Request

Input Parameter:
.  MPI_Request request - request handle

Output Parameters:
+  int *flag - true if operation has completed (logical)
-  MPI_Status *status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

   Notes:

   Unlike MPI_Test, MPI_Request_get_status does not deallocate or deactivate
   the request.  A call to one of the test/wait routines or MPI_Request_free
   should be made to release the request object.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Request_get_status";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REQUEST_GET_STATUS);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REQUEST_GET_STATUS);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_REQUEST(request, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    if (mpi_errno) {
		goto fn_exit;
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( request, request_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate request_ptr */
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) {
		goto fn_exit;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (*request_ptr->cc_ptr == 0)
    {
	switch(request_ptr->kind)
	{
	    case MPID_REQUEST_SEND:
	    case MPID_REQUEST_RECV:
	    {
		mpi_errno = request_ptr->status.MPI_ERROR;
		if (status != MPI_STATUS_IGNORE)
		{
		    *status = request_ptr->status;
		}
		break;
	    }
			
	    case MPID_PREQUEST_SEND:
	    case MPID_PREQUEST_RECV:
	    {
		if (request_ptr->partner_request != NULL)
		{
		    mpi_errno = request_ptr->partner_request->status.MPI_ERROR;
		    if (status != MPI_STATUS_IGNORE)
		    {
			*status = request_ptr->partner_request->status;
		    }
		}
		else
		{
		    if (request_ptr->status.MPI_ERROR != MPI_SUCCESS)
		    {
			/* if the persistent request failed to start then make the error code available */
			mpi_errno = request_ptr->status.MPI_ERROR;
			if (status != MPI_STATUS_IGNORE)
			{
			    *status = request_ptr->status;
			}
		    }
		    else
		    {
			MPIR_Status_set_empty(status);
		    }
		}
	    
		break;
	    }

	    case MPID_UREQUEST:
	    {
		mpi_errno = (request_ptr->query_fn)(request_ptr->grequest_extra_state, &request_ptr->status);
		if (mpi_errno == MPI_SUCCESS)
		{
		    mpi_errno = request_ptr->status.MPI_ERROR;
		}
	    
		if (status != MPI_STATUS_IGNORE)
		{
		    *status = request_ptr->status;
		}
		
		break;
	    }
	}

	
	*flag = TRUE;
    }
    else
    {
	*flag = FALSE;
    }
    
  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_GET_STATUS);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS : MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
}

