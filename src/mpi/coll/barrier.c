/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Barrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Barrier = PMPI_Barrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Barrier  MPI_Barrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Barrier as PMPI_Barrier
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Barrier PMPI_Barrier

/* This is the default implementation of the barrier operation.  The
   algorithm is:
   
   Algorithm: MPI_Barrier

   Find the largest power of two that is less than or equal to the size of 
   the communicator.  Call tbis twon_within.

   Divide the communicator by rank into two groups: those with 
   rank < twon_within and those with greater rank.  The barrier
   executes in three steps.  First, the group with rank >= twon_within
   sends to the first (size-twon_within) ranks of the first group.
   That group then executes a recursive doubling algorithm for the barrier.
   For the third step, the first (size-twon_within) ranks send to the top
   group.  This is the same algorithm used in MPICH-1.

   Possible improvements: 
   The upper group could apply recursively this approach to reduce the 
   total number of messages sent (in the case of of a size of 2^n-1, there 
   are 2^(n-1) messages sent in the first and third steps).

   End Algorithm: MPI_Barrier

   This is an intracommunicator barrier only!
*/
PMPI_LOCAL int MPIR_Barrier( MPID_Comm *comm_ptr )
{
    int size, rank;
    int twon_within, n2, remaining, gap, partner;
    MPID_Request *request_ptr;

    size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Trivial barriers return immediately */
    if (size == 1) return MPI_SUCCESS;

    /* Only one collective operation per communicator can be active at any
       time */
    MPID_Comm_thread_lock( comm_ptr );
    
    /* Find the twon_within (this could be cached if more routines
     need it) */
    twon_within = 1;
    n2          = 2;
    while (n2 <= size) { twon_within = n2; n2 <<= 1; }
    remaining = size - twon_within;

    if (rank < twon_within) {
	/* First step: receive from the upper group */
	if (rank < remaining) {
	    MPID_Recv( 0, 0, MPI_BYTE, twon_within + rank, MPIR_BARRIER_TAG, 
		       comm_ptr, MPID_CONTEXT_INTRA_COLL, 0, &request_ptr );
	    if (request_ptr) {
		MPIR_Wait(request_ptr);
		MPID_Request_release(request_ptr);
	    }
	}
	/* Second step: recursive doubling exchange */
	for (gap=1; gap<twon_within; gap <<= 1) {
	    partner = (rank ^ gap);
	    if (rank < partner) {
		MPID_Recv( 0, 0, MPI_BYTE, partner, MPIR_BARRIER_TAG, 
			   comm_ptr, MPID_CONTEXT_INTRA_COLL, 0, 
			   &request_ptr );
		if (request_ptr) {
		    MPIR_Wait(request_ptr);
		    MPID_Request_release(request_ptr);
		}
	    }
	    else {
		MPID_Send( 0, 0, MPI_BYTE, partner, MPIR_BARRIER_TAG,
			   comm_ptr, MPID_CONTEXT_INTRA_COLL, &request_ptr );
		if (request_ptr) {
		    MPIR_Wait(request_ptr);
		    MPID_Request_release(request_ptr);
		}
	    }
	}

	/* Third step: send to the upper group */
	if (rank < remaining) {
	    MPID_Send( 0, 0, MPI_BYTE, rank + twon_within, MPIR_BARRIER_TAG,
		       comm_ptr, MPID_CONTEXT_INTRA_COLL, &request_ptr );
	    if (request_ptr) {
		MPIR_Wait(request_ptr);
		MPID_Request_release(request_ptr);
	    }
	}
    }
    else {
	/* For the upper group, step one is a send */
	MPID_Send( 0, 0, MPI_BYTE, rank - twon_within, MPIR_BARRIER_TAG,
		   comm_ptr, MPID_CONTEXT_INTRA_COLL, &request_ptr );
	if (request_ptr) {
	    MPIR_Wait(request_ptr);
	    MPID_Request_release(request_ptr);
	}
	/* There is no second step; for the third step, recv */
	MPID_Recv( 0, 0, MPI_BYTE, rank - twon_within, MPIR_BARRIER_TAG, 
		   comm_ptr, MPID_CONTEXT_INTRA_COLL, 0, &request_ptr );
	if (request_ptr) {
	    MPIR_Wait(request_ptr);
	    MPID_Request_release(request_ptr);
	}
    }

    MPID_Comm_thread_unlock( comm_ptr );

    return MPI_SUCCESS;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Barrier

/*@
   MPI_Barrier - barrier

   Arguments:
.  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Barrier( MPI_Comm comm )
{
    static const char FCNAME[] = "MPI_Barrier";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BARRIER);

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

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_BARRIER);
    
    /* Convert handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Barrier != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Barrier(comm_ptr);
    }
    else
    {
	mpi_errno = MPIR_Barrier( comm_ptr );
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BARRIER);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    /* ... end of body of routine ... */
}
