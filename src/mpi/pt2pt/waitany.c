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

/* -- Begin Profiling Symbol Block for routine MPI_Waitany */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitany = PMPI_Waitany
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitany  MPI_Waitany
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitany as PMPI_Waitany
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Waitany PMPI_Waitany

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitany

/*@
    MPI_Waitany - Waits for any specified send or receive to complete

Input Parameters:
+ count - list length (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ index - index of handle for operation that completed (integer).  In the
range '0' to 'count-1'.  In Fortran, the range is '1' to 'count'.
- status - status object (Status).  May be 'MPI_STATUS_IGNORE'.

Notes:
If all of the requests are 'MPI_REQUEST_NULL', then 'index' is returned as 
'MPI_UNDEFINED', and 'status' is returned as an empty status.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
@*/
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *index, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Waitany";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = NULL;
    int i;
    int n_inactive;
    int active_flag;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITANY);

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
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITANY);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests",
				 mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    if (array_of_requests != NULL && count > 0)
	    {
		for (i = 0; i < count; i++)
		{
		    MPIR_ERRTEST_REQUEST(array_of_requests[i], mpi_errno);
		}
	    }
            if (mpi_errno) {
                goto fn_exit;
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
	/* --BEGIN ERROR HANDLING-- */
	if (request_ptrs == NULL)
	{
	    mpi_errno = MPIR_ERR_MEMALLOCFAILED;
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */
    }

    n_inactive = 0;
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
		    if (mpi_errno) {
			goto fn_exit;
		    }
		    
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif	    
	}
	else
	{
	    request_ptrs[i] = NULL;
	    n_inactive += 1;
	}
    }

    if (n_inactive == count)
    {
	*index = MPI_UNDEFINED;
	MPIR_Status_set_empty(status);
	goto fn_exit;
    }
    
    for(;;)
    {
	MPID_Progress_start();

	for (i = 0; i < count; i++)
	{
	    if (request_ptrs[i] != NULL && *request_ptrs[i]->cc_ptr == 0)
	    {
		mpi_errno = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status, &active_flag);
		if (active_flag)
		{
		    MPID_Progress_end();
		    *index = i;
		    goto break_l1;
		}
		else
		{
		    n_inactive += 1;
		    request_ptrs[i] = NULL;

		    if (n_inactive == count)
		    {
			MPID_Progress_end();
			*index = MPI_UNDEFINED;
			/* status is set to empty by MPIR_Request_complete */
			goto break_l1;
		    }
		}
	    }
	}

	mpi_errno = MPID_Progress_wait();
	if (mpi_errno != MPI_SUCCESS)
	{
	    goto fn_exit;
	}
    }
  break_l1:

  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITANY);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS : MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
