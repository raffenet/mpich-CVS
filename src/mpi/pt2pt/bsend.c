/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Bsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bsend = PMPI_Bsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bsend  MPI_Bsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bsend as PMPI_Bsend
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Bsend PMPI_Bsend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Bsend

/*@
    MPI_Bsend - Basic send with user-specified buffering

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (nonnegative integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
This send is provided as a convenience function; it allows the user to 
send messages without worring about where they are buffered (because the
user `must` have provided buffer space with 'MPI_Buffer_attach').  

In deciding how much buffer space to allocate, remember that the buffer space 
is not available for reuse by subsequent 'MPI_Bsend's unless you are certain 
that the message
has been received (not just that it should have been received).  For example,
this code does not allocate enough buffer space
.vb
    MPI_Buffer_attach( b, n*sizeof(double) + MPI_BSEND_OVERHEAD );
    for (i=0; i<m; i++) {
        MPI_Bsend( buf, n, MPI_DOUBLE, ... );
    }
.ve
because only enough buffer space is provided for a single send, and the
loop may start a second 'MPI_Bsend' before the first is done making use of the
buffer.  

In C, you can 
force the messages to be delivered by 
.vb
    MPI_Buffer_detach( &b, &n );
    MPI_Buffer_attach( b, n );
.ve
(The 'MPI_Buffer_detach' will not complete until all buffered messages are 
delivered.)

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_RANK
.N MPI_ERR_TAG

.seealso: MPI_Buffer_attach, MPI_Ibsend, MPI_Bsend_init
@*/
int MPI_Bsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	      MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Bsend";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BSEND);

    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_BSEND);
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
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
#ifdef MPID_HAS_TBSEND
    mpi_errno = MPID_tBsend( buf, count, datatype, dest, tag, comm_ptr, 0 );
    if (mpi_errno == MPI_SUCCESS) {
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
	return MPI_SUCCESS;
    }
    /* Check for MPID_WOULD_BLOCK? */
#endif    
    mpi_errno = MPIR_Bsend_isend( buf, count, datatype, dest, tag, comm_ptr, 
				  &request_ptr );
    if (mpi_errno) {
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    /* We'll wait on the request, if any, within the bsendutil.c functions
       that advance active sends */
    /* ... end of body of routine ... */

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
    return MPI_SUCCESS;
}
