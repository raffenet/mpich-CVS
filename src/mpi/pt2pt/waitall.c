/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if !defined(MPID_REQUEST_PTR_ARRAY_SIZE)
#define MPID_REQUEST_PTR_ARRAY_SIZE 16
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Waitall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitall = PMPI_Waitall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitall  MPI_Waitall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitall as PMPI_Waitall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Waitall PMPI_Waitall

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitall

/*@
    MPI_Waitall - Waits for all given MPI Requests to complete

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of request handles (array of handles)

Output Parameter:
. array_of_statuses - array of status objects (array of Statuses).  May be
  'MPI_STATUSES_IGNORE'.

Notes:

If one or more of the requests completes with an error, 'MPI_ERR_IN_STATUS' is
returned.  An error value will be present is elements of 'array_of_status'
associated with the requests.  Likewise, the 'MPI_ERROR' field in the status
elements associated with requests that have successfully completed will be
'MPI_SUCCESS'.  Finally, those requests that have not completed will have a 
value of 'MPI_ERR_PENDING'.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitall(int count, MPI_Request array_of_requests[], 
		MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Waitall";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = NULL;
    MPI_Status * status_ptr;
    MPID_Progress_state progress_state;
    int i;
    int n_completed;
    int active_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITALL);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno)
	    {
		mpi_errno = MPIR_Err_create_code(
		    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
		    MPI_ERR_OTHER, "**mpi_waitall",
		    "**mpi_waitall %d %p %p", count, array_of_requests,
		    array_of_statuses);
		return MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITALL);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    if (count != 0) 
	    {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", 
				     mpi_errno);
		/* NOTE: MPI_STATUSES_IGNORE != NULL */
		MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses",
				     mpi_errno);
		if (array_of_requests != NULL && count > 0)
		{
		    for (i = 0; i < count; i++)
		    {
			MPIR_ERRTEST_REQUEST(array_of_requests[i], mpi_errno);
		    }
		}
	    }
            if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING -- */
		goto fn_fail;
		/* --END ERROR HANDLING -- */
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI request handles to a request object pointers */
    if (count <= MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	request_ptrs = request_ptr_array;
    }
    else
    {
	request_ptrs = MPIU_Malloc(count * sizeof(MPID_Request *));
	if (request_ptrs == NULL)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
					     FCNAME, __LINE__, MPI_ERR_OTHER, 
					     "**nomem",
					     "**nomem %d", 
					     count * sizeof(MPID_Request*));
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
    }

    n_completed = 0;
    for (i = 0; i < count; i++)
    {
	if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	    MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
	    /* Validate object pointers if error checking is enabled */
#           ifdef HAVE_ERROR_CHECKING
	    {
		MPID_BEGIN_ERROR_CHECKS;
		{
		    MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
		    if (mpi_errno != MPI_SUCCESS)
		    {
			goto fn_fail;
		    }
		    
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif	    
	}
	else
	{
	    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
	    MPIR_Status_set_empty(status_ptr);
	    request_ptrs[i] = NULL;
	    n_completed += 1;
	}
    }
    
    if (n_completed == count)
    {
	goto fn_exit;
    }
    
    MPID_Progress_start(&progress_state);
    for(;;)
    {
	for (i = 0; i < count; i++)
	{
	    if (request_ptrs[i] != NULL && *request_ptrs[i]->cc_ptr == 0)
	    {
		status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[i] : MPI_STATUS_IGNORE;
		rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
		if (rc == MPI_SUCCESS) 
		{ 
		    request_ptrs[i] = NULL;
		}
		else
		{
		    mpi_errno = MPI_ERR_IN_STATUS;
		    if (status_ptr != MPI_STATUS_IGNORE)
		    { 
			status_ptr->MPI_ERROR = rc;
		    }
		}
		
		n_completed += 1;
	    }
	}
	
	if (mpi_errno == MPI_ERR_IN_STATUS)
	{
	    if (array_of_statuses != MPI_STATUSES_IGNORE)
	    {
		for (i = 0; i < count; i++)
		{
		    if (request_ptrs[i] == NULL)
		    {
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		    }
		    else
		    {
			if (array_of_requests[i] != MPI_REQUEST_NULL)
			{ 
			    array_of_statuses[i].MPI_ERROR = MPI_ERR_PENDING;
			}
		    }
		}
	    }
	    
	    break;
	}
	else if (n_completed == count)
	{
	    break;
	}

	mpi_errno = MPID_Progress_wait(&progress_state);
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPID_Progress_end(&progress_state);
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
    }
    MPID_Progress_end(&progress_state);

  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITALL);
    return mpi_errno;

  fn_fail:
#ifdef HAVE_ERROR_CHECKING
    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_waitall", 
				     "**mpi_waitall %d %p %p", 
				     count, array_of_requests, 
				     array_of_statuses);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

