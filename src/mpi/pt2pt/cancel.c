/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cancel */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cancel = PMPI_Cancel
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cancel  MPI_Cancel
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cancel as PMPI_Cancel
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Cancel PMPI_Cancel

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cancel

/*@
    MPI_Cancel - Cancels a communication request

Input Parameter:
. request - communication request (handle) 

Notes:
The primary expected use of 'MPI_Cancel' is in multi-buffering
schemes, where speculative 'MPI_Irecvs' are made.  When the computation 
completes, some of these receive requests may remain; using 'MPI_Cancel' allows
the user to cancel these unsatisfied requests.  

Cancelling a send operation is much more difficult, in large part because the 
send will usually be at least partially complete (the information on the tag,
size, and source are usually sent immediately to the destination).  
Users are
advised that cancelling a send, while a local operation (as defined by the MPI
standard), is likely to be expensive (usually generating one or more internal
messages). 

.N ThreadSafe

.N Fortran

.N NULL

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Cancel(MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Cancel";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CANCEL);

    MPIR_ERRTEST_INITIALIZED_ORRETURN();
    
    MPID_CS_ENTER();
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_CANCEL);
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( *request, request_ptr );
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate request_ptr */
            MPID_Request_valid_ptr( request_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    switch (request_ptr->kind)
    {
	case MPID_REQUEST_SEND:
	{
	    mpi_errno = MPID_Cancel_send(request_ptr);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno) goto fn_fail;
	    /* --END ERROR HANDLING-- */
	    break;
	}

	case MPID_REQUEST_RECV:
	{
	    mpi_errno = MPID_Cancel_recv(request_ptr);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno) goto fn_fail;
	    /* --END ERROR HANDLING-- */
	    break;
	}

	case MPID_PREQUEST_SEND:
	{
	    if (request_ptr->partner_request != NULL)
	    {
		mpi_errno = MPID_Cancel_send(request_ptr->partner_request);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno) goto fn_fail;
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_REQUEST,
						  "**requestpersistactive", NULL);
	    }
	    /* --END ERROR HANDLING-- */
	    
	    break;
	}

	case MPID_PREQUEST_RECV:
	{
	    if (request_ptr->partner_request != NULL)
	    {
		mpi_errno = MPID_Cancel_recv(request_ptr->partner_request);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno) goto fn_fail;
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_REQUEST,
						  "**requestpersistactive", NULL);
	    }
	    /* --END ERROR HANDLING-- */

	    break;
	}

	case MPID_UREQUEST:
	{
	    mpi_errno = (request_ptr->cancel_fn)(request_ptr->grequest_extra_state, (request_ptr->cc == 0));
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**user",
						 "**usercancel %d", mpi_errno);
		goto fn_fail;
	    }
	    /* --END ERROR HANDLING-- */
	    break;
	}

	default:
	{
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_INTERN,
					      "**cancelunknown", NULL );
	    goto fn_fail;
	}
    }

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
    MPID_CS_EXIT();
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_cancel", "**mpi_cancel %p", request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
