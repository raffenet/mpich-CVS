/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Request_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Request_free = PMPI_Request_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Request_free  MPI_Request_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Request_free as PMPI_Request_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Request_free PMPI_Request_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Request_free

/*@
    MPI_Request_free - Frees a communication request object

Input Parameter:
. request - communication request (handle) 

Notes:

This routine is normally used to free inactive persistent requests created with
either 'MPI_Recv_init' or 'MPI_Send_init' and friends.  It `is` also
permissible to free an active request.  However, once freed, the request can no
longer be used in a wait or test routine (e.g., 'MPI_Wait') to determine
completion.

This routine may also be used to free a non-persistent requests such as those
created with 'MPI_Irecv' or 'MPI_Isend' and friends.  Like active persistent
requests, once freed, the request can no longer be used with test/wait routines
to determine completion.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG

.see also: MPI_Isend, MPI_Irecv, MPI_Issend, MPI_Ibsend, MPI_Irsend,
MPI_Recv_init, MPI_Send_init, MPI_Ssend_init, MPI_Rsend_init, MPI_Wait,
MPI_Test, MPI_Waitall, MPI_Waitany, MPI_Waitsome, MPI_Testall, MPI_Testany,
MPI_Testsome
@*/
int MPI_Request_free(MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Request_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REQUEST_FREE);

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
	    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REQUEST_FREE);
    
    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
	    if (request != NULL)
	    {
		MPIR_ERRTEST_REQUEST(*request, mpi_errno);
	    }
	    if (mpi_errno) {
		goto fn_exit;
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr( *request, request_ptr );

    /* Validate object pointers if error checking is enabled */
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
    
    switch (request_ptr->kind)
    {
	case MPID_REQUEST_SEND:
	case MPID_REQUEST_RECV:
	{
	    break;
	}
	
	case MPID_PREQUEST_SEND:
	case MPID_PREQUEST_RECV:
	{
	    /* If this is an active persistent request, we must also release the partner request. */
	    if (request_ptr->partner_request != NULL)
	    {
		MPID_Request_release(request_ptr->partner_request);
	    }
	    break;
	}
	
	case MPID_UREQUEST:
	{
	    switch (request_ptr->greq_lang) {
	    case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING:
	    case MPID_LANG_CXX:
#endif
	    mpi_errno = (request_ptr->free_fn)(request_ptr->grequest_extra_state);
	    break;
#ifdef HAVE_FORTRAN_BINDING:
	    case MPID_LANG_FORTRAN:
	    case MPID_LANG_FORTRAN90:
	    {
		MPI_Fint ierr;
		( (MPIR_Grequest_f77_free_function*)(request_ptr->free_fn))(
		    request_ptr->grequest_extra_state, 
		    &ierr );
		mpi_errno = (int) ierr;
	    }
	    break;
#endif	    
	    }
	    break;
	}
    }

    MPID_Request_release(request_ptr);
    *request = MPI_REQUEST_NULL;

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_request_free", "**mpi_request_free %p", request);
    }
#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REQUEST_FREE);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS : MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
