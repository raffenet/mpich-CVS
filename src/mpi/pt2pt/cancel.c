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
   MPI_Cancel - cancel

   Arguments:
.  MPI_Request *request - request

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Cancel(MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Cancel";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * request_ptr;
    MPID_MPI_STATE_DECLS;

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
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    switch (request_ptr->kind)
    {
	case MPID_REQUEST_RECV:
	{
	    MPID_Cancel_recv(request_ptr);
	    break;
	}

	case MPID_PREQUEST_RECV:
	{
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, 
					      "**cancelperrecv", 0 );
	    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	}

	case MPID_REQUEST_SEND:
	{
	    MPID_Cancel_send(request_ptr);
	    break;
	}

	case MPID_PREQUEST_SEND:
	{
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, 
					      "**cancelpersend", 0 );
	    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	}

	case MPID_UREQUEST:
	{
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
	    mpi_errno = (request_ptr->cancel_fn)( 
		request_ptr->grequest_extra_state, 
		(request_ptr->cc == 0) );
	    break;
	}

	default:
	{
	    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
	    mpi_errno = MPIR_Err_create_code(
		MPI_ERR_INTERN, "**cancelunknown", 0 );
	    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	}
    }
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_CANCEL);
    return MPI_SUCCESS;
}
