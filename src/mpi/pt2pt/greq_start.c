/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Grequest_start */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Grequest_start = PMPI_Grequest_start
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Grequest_start  MPI_Grequest_start
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Grequest_start as PMPI_Grequest_start
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Grequest_start PMPI_Grequest_start

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of 
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Grequest_start

/*@
   MPI_Grequest_start - Create and return a user-defined request

Input Parameters:
+ query_fn - callback function invoked when request status is queried (function)  
. free_fn - callback function invoked when request is freed (function) 
. cancel_fn - callback function invoked when request is cancelled (function) 
- extra_state - Extra state passed to the above functions.

Output Parameter:
.  request - Generalized request (handle)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Grequest_start( MPI_Grequest_query_function *query_fn, 
			MPI_Grequest_free_function *free_fn, 
			MPI_Grequest_cancel_function *cancel_fn, 
			void *extra_state, MPI_Request *request )
{
    static const char FCNAME[] = "MPI_Grequest_start";
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *lrequest_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GREQUEST_START);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);
	    if (request != NULL)
	    {
		MPIR_ERRTEST_REQUEST(*request, mpi_errno);
	    }
            if (mpi_errno) {
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GREQUEST_START);

    /* ... body of routine ...  */
    lrequest_ptr = MPID_Request_create();
    if (!lrequest_ptr) {
        MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GREQUEST_START);
	return MPI_ERR_NOMEM;
    }
    lrequest_ptr->kind                 = MPID_UREQUEST;
    MPIU_Object_set_ref( lrequest_ptr, 2 );
    lrequest_ptr->cc_ptr               = &lrequest_ptr->cc;
    lrequest_ptr->cc                   = 1;
    lrequest_ptr->comm                 = NULL;
    lrequest_ptr->cancel_fn            = cancel_fn;
    lrequest_ptr->free_fn              = free_fn;
    lrequest_ptr->query_fn             = query_fn;
    lrequest_ptr->grequest_extra_state = extra_state;

    /* NOTE: the request is given a ref_count of 2 so that the object is not
       destroyed until both MPI_Grequest_complete() and a communication
       completion routine (e.g., MPI_Wait()) is called. */
    
    *request = lrequest_ptr->handle;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GREQUEST_START);
    return MPI_SUCCESS;
}
