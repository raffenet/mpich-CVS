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
- status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

.N waitstatus

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Test";
    MPID_Request *request_ptr = NULL;
    int active_flag;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TEST);

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

    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TEST);

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
    
    /* If this is a null request handle, then return an empty status */
    if (*request == MPI_REQUEST_NULL)
    {
	if (status) {
	    MPIR_Status_set_empty(status);
	}
	*flag = TRUE;
	goto fn_exit;
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
		goto fn_exit;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    *flag = FALSE;

#   if defined(USE_MPID_PROGRESS_AVOIDANCE)
    {
	if (*request_ptr->cc_ptr == 0)
	{
	    mpi_errno = MPIR_Request_complete(request, request_ptr, status, &active_flag);
	    *flag = TRUE;
	    goto fn_exit;
	}
    }
#   endif    

    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    
    if (*request_ptr->cc_ptr == 0)
    {
	mpi_errno = MPIR_Request_complete(request, request_ptr, status, &active_flag);
	*flag = TRUE;
	goto fn_exit;
    }
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TEST);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS : MPIR_Err_return_comm(request_ptr?request_ptr->comm:0, FCNAME, mpi_errno);
}
