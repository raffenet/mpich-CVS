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
    int i, k;
    MPID_Request *request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TESTANY);

    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_TESTANY);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(index,"index",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(flag,"flag",mpi_errno);
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TESTANY);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Iterate at most twice; the second is after a progress_test, if needed */
    *flag = 0;
    for (k=0; k<2; k++) {
	MPID_Progress_start();
	for (i=0; i<count; i++) {
	    /* Get handles to MPI objects. */
	    if (array_of_requests[i] == MPI_REQUEST_NULL) {
		continue;
	    }
	    MPID_Request_get_ptr( array_of_requests[i], request_ptr );
	    
	    if ((request_ptr->cc_ptr) == 0) {
		/* Found one */
		*index = i;
		*flag  = 1;
		if (status != NULL && 
		    (request_ptr->kind == MPID_REQUEST_RECV ||
		     request_ptr->kind == MPID_PREQUEST_RECV)) {
		    *status = request_ptr->status;
		}
		array_of_requests[i] = MPI_REQUEST_NULL;
		break;
	    }
	}
	if (i == count) 
	    MPID_Progress_test();
	else {
	    MPID_Progress_end();
	    break;
	}
    }
    /* ... end of body of routine ... */
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_TESTANY);
    return MPI_SUCCESS;
}
