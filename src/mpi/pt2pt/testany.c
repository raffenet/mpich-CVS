/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Testany */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Testany = PMPI_Testany
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Testany  MPI_Testany
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Testany as PMPI_Testany
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Testany PMPI_Testany

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Testany

/*@
    MPI_Testany - Tests for completion of any previdously initiated 
                  communication

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ index - index of operation that completed, or 'MPI_UNDEFINED'  if none 
  completed (integer) 
. flag - true if one of the operations is complete (logical) 
- status - status object (Status).  May be 'MPI_STATUS_NULL'.

.N waitstatus

.N fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Testany(int count, MPI_Request array_of_requests[], int *index, 
		int *flag, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Testany";
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TESTANY);

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
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TESTANY);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests",
				 mpi_errno);
	    MPIR_ERRTEST_ARGNULL(index, "index", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag, "flag", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
            if (mpi_errno) {
                goto fn_exit;
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#   if !defined(USE_MPID_PROGRESS_AVOIDANCE)
    {
	MPID_Progress_test();
    }
#   endif

    *flag = FALSE;
    *index = MPI_UNDEFINED;
    
    for (i = 0; i < count; i++)
    {
	MPID_Request * request_ptr;
	
	MPID_Request_get_ptr(array_of_requests[i], request_ptr);
	if ((*request_ptr->cc_ptr) == 0)
	{
	    mpi_errno = MPIR_Request_complete(
		&array_of_requests[i], request_ptr, status);
	    *index = i;
	    *flag = TRUE;
	    goto fn_exit;
	}
    }

#   if defined(USE_MPID_PROGRESS_AVOIDANCE)
    {
	MPID_Progress_test();
	
	for (i = 0; i < count; i++)
	{
	    MPID_Request * request_ptr;
	    
	    MPID_Request_get_ptr(array_of_requests[i], request_ptr);
	    if ((*request_ptr->cc_ptr) == 0)
	    {
		mpi_errno = MPIR_Request_complete(
		    &array_of_requests[i], request_ptr, status);
		*index = i;
		*flag = TRUE;
		goto fn_exit;
	    }
	}
    }
#   endif
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TESTANY);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
