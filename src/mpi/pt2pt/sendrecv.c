/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Sendrecv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Sendrecv  MPI_Sendrecv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Sendrecv as PMPI_Sendrecv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Sendrecv PMPI_Sendrecv

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Sendrecv

/*@
   MPI_Sendrecv - sendrecv

   Arguments:
+  void *sendbuf - send buffer
.  int sendcount - send count
.  MPI_Datatype sendtype - send datatype
.  int dest - destination
.  int sendtag - send tag
.  void *recvbuf - receive buffer
.  int recvcount - receive count
.  MPI_Datatype recvtype - receive datatype
.  int source - source
.  int recvtag - receive tag
.  MPI_Comm comm - communicator
-  MPI_Status *status - status

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Sendrecv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * reqs[2];
    MPID_MPI_STATE_DECLS;
    
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
	    
    MPID_MPI_PT2PT_FUNC_ENTER_BOTH(MPID_STATE_MPI_SENDRECV);
    
    /* Convert handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype * sendtype_ptr = NULL;
	    MPID_Datatype * recvtype_ptr = NULL;
	    
	    
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SENDRECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    
            /* Validate datatypes */
	    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
	    MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
            MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
            MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SENDRECV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Irecv(recvbuf, recvcount, recvtype, source, recvtag,
			   comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &reqs[0]);
    if (mpi_errno)
    {
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SENDRECV);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* XXX: Performance for small messages might be better if MPID_Send() were
       used here instead of MPID_Isend() */
    mpi_errno = MPID_Isend(sendbuf, sendcount, sendtype, dest, sendtag,
			   comm_ptr, MPID_CONTEXT_INTRA_PT2PT, &reqs[1]);
    if (mpi_errno)
    {
	MPID_Request_release(reqs[0]);
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SENDRECV);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    while(1)
    {
	MPID_Progress_start();
	
	if (*reqs[0]->cc_ptr != 0 || *reqs[1]->cc_ptr != 0)
	{
	    MPID_Progress_wait();
	}
	else
	{
	    MPID_Progress_end();
	    break;
	}
    }

    if (status != NULL)
    {
	*status = reqs[0]->status;
    }

    MPID_Request_release(reqs[0]);
    MPID_Request_release(reqs[1]);
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SENDRECV);
    return MPI_SUCCESS;
}
