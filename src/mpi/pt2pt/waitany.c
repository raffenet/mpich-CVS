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
- status - status object (Status).  May be 'MPI_STATUS_NULL'.

Notes:
If all of the requests are 'MPI_REQUEST_NULL', then 'index' is returned as 
'MPI_UNDEFINED', and 'status' is returned as an empty status.

.N waitstatus

.N fortran

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
	if (request_ptrs == NULL)
	{
	    mpi_errno = MPI_ERR_NOMEM;
	    goto fn_exit;
	}
    }

    for (i = 0; i < count; i++)
    {
	MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
    }
    
    /* Validate object pointers if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    for (i = 0; i < count; i++)
	    {
		MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
	    }
            if (mpi_errno) {
		goto fn_exit;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

#   if defined(USE_MPID_PROGRESS_AVOIDANCE)
    {
	for (i = 0; i < count; i++)
	{
	    if ((*request_ptrs[i]->cc_ptr) == 0)
	    {
		mpi_errno = MPIR_Request_complete(
		    &array_of_requests[i], request_ptrs[i], status);
		*index = i;
		goto break_l1;
	    }
	}
    }
#   endif
    
    for(;;)
    {
	MPID_Progress_start();

	for (i = 0; i < count; i++)
	{
	    if ((*request_ptrs[i]->cc_ptr) == 0)
	    {
		mpi_errno = MPIR_Request_complete(
		    &array_of_requests[i], request_ptrs[i], status);
		*index = i;
		MPID_Progress_end();
		goto break_l1;
	    }
	}

	MPID_Progress_wait();
    }
  break_l1:
    
  fn_exit:
    if (request_ptrs != request_ptr_array && request_ptrs != NULL)
    {
	MPIU_Free(request_ptrs);
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITANY);
    return (mpi_errno == MPI_SUCCESS) ? MPI_SUCCESS :
	MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
}
