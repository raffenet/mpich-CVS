/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ibsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibsend = PMPI_Ibsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibsend  MPI_Ibsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibsend as PMPI_Ibsend
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Ibsend PMPI_Ibsend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Ibsend

/*@
    MPI_Ibsend - Starts a nonblocking buffered send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Output Parameter:
. request - communication request (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_BUFFER

@*/
int MPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	       MPI_Comm comm, MPI_Request *request)
{
    static const char FCNAME[] = "MPI_Ibsend";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IBSEND);

    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_IBSEND);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count,mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr) {
		MPIR_ERRTEST_SEND_TAG(tag,mpi_errno);
		MPIR_ERRTEST_SEND_RANK(comm_ptr,dest,mpi_errno)
	    }

	    /* Validate datatype */
	    MPIR_ERRTEST_DATATYPE(count,datatype,mpi_errno);

	    /* Validate buffer */
	    MPIR_ERRTEST_USERBUFFER(buf,count,datatype,mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* We don't try tbsend in for MPI_Ibsend because we must create a
       request even if we can send the message */

    mpi_errno = MPIR_Bsend_isend( buf, count, datatype, dest, tag, comm_ptr, 
				  IBSEND, &request_ptr );
    if (mpi_errno == MPI_SUCCESS)
    {
	/* FIXME.  For now, we'll assume that the all the message has
	   been copied, so we can return a created request here */
	/* Create a completed request */
	request_ptr = MPID_Request_create();
	MPIU_ERR_CHKANDJUMP1(!request_ptr,mpi_errno,MPI_ERR_OTHER,
			     "**nomem", "**nomem %s", "MPI_Request");

	request_ptr->kind                 = MPID_REQUEST_SEND;
	MPIU_Object_set_ref( request_ptr, 1 );
	request_ptr->cc_ptr               = &request_ptr->cc;
	request_ptr->cc                   = 0;
	request_ptr->comm                 = NULL;
	*request = request_ptr->handle;
    }
    else
    {
	*request = MPI_REQUEST_NULL;
	goto fn_fail;
    }
    /* ... end of body of routine ... */

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IBSEND);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_HANDLING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_ibsend", "**mpi_ibsend %p %d %D %d %d %C %p", buf, count, datatype, dest, tag, comm, request);
#endif
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IBSEND);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
