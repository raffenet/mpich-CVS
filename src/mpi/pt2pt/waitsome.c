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

/* -- Begin Profiling Symbol Block for routine MPI_Waitsome */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitsome = PMPI_Waitsome
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitsome  MPI_Waitsome
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitsome as PMPI_Waitsome
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Waitsome PMPI_Waitsome

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitsome

/*@
    MPI_Waitsome - Waits for some given communications to complete

Input Parameters:
+ incount - length of array_of_requests (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ outcount - number of completed requests (integer) 
. array_of_indices - array of indices of operations that 
completed (array of integers) 
- array_of_statuses - array of status objects for 
    operations that completed (array of Status).  May be 'MPI_STATUSES_NULL'.

Notes:
  The array of indicies are in the range '0' to 'incount - 1' for C and 
in the range '1' to 'incount' for Fortran.  

Null requests are ignored; if all requests are null, then the routine
returns with 'outcount' set to 'MPI_UNDEFINED'.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

.N waitstatus

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Waitsome";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = NULL;
    MPI_Status * status_ptr;
    int i;
    int n_active;
    int n_inactive;
    int active_flag;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITSOME);

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
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITSOME);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(incount, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests",
				 mpi_errno);
	    MPIR_ERRTEST_ARGNULL(outcount, "outcount", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_indices, "array_of_indices",
				 mpi_errno);
	    /* NOTE: MPI_STATUSES_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses",
				 mpi_errno);
	    if (array_of_requests != NULL && incount > 0)
	    {
		for (i = 0; i < incount; i++)
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
    
    *outcount = 0;
    
    /* Convert MPI request handles to a request object pointers */
    if (incount <= MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	request_ptrs = request_ptr_array;
    }
    else
    {
	request_ptrs = MPIU_Malloc(incount * sizeof(MPID_Request *));
	if (request_ptrs == NULL)
	{
	    mpi_errno = MPI_ERR_NOMEM;
	    goto fn_exit;
	}
    }
    
    n_inactive = 0;
    for (i = 0; i < incount; i++)
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
	    n_inactive += 1;
	    request_ptrs[i] = NULL;
	} 
    }

    if (n_inactive == incount)
    {
	*outcount = MPI_UNDEFINED;
	goto fn_exit;
    }
    
    /* Bill Gropp says MPI_Waitsome() is expected to try to make progress even
       if some requests have already completed; therefore, we kick the pipes
       once and then fall into a loop checking for completion and waiting for
       progress. */
    MPID_Progress_test();

    n_active = 0;
    for(;;)
    {
	MPID_Progress_start();

	for (i = 0; i < incount; i++)
	{
	    if (request_ptrs[i] != NULL && *request_ptrs[i]->cc_ptr == 0)
	    {
		status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ?
		    &array_of_statuses[n_active] : MPI_STATUS_IGNORE;
		rc = MPIR_Request_complete(&array_of_requests[i],
					   request_ptrs[i], status_ptr,
					   &active_flag);
		if (active_flag)
		{
		    array_of_indices[n_active] = i;
		    n_active += 1;
		    
		    if (rc != MPI_SUCCESS)
		    {
			mpi_errno = MPI_ERR_IN_STATUS;
		    }
		}
		else
		{
		    n_inactive += 1;
		}
		
		request_ptrs[i] = NULL;
	    }
	}
	
	if (n_active > 0)
	{
	    MPID_Progress_end();
	    *outcount = n_active;
	    break;
	}
	else if (n_inactive == incount)
	{
	    MPID_Progress_end();
	    *outcount = MPI_UNDEFINED;
	    break;
	}

	MPID_Progress_wait();
    }


  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITSOME);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
