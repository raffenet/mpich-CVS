/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Alltoallv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoallv = PMPI_Alltoallv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoallv  MPI_Alltoallv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoallv as PMPI_Alltoallv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Alltoallv PMPI_Alltoallv
/* This is the default implementation of alltoallv. The algorithm is:
   
   Algorithm: MPI_Alltoallv

   For both short and long messages, we use the same pairwise exchange
   algorithm used in MPI_Alltoall for long messages. We don't use the
   recursive doubling algorithm for short messages because a process
   doesn't know any other process's sendcounts or recvcounts arrays.
   Cost = (p-1).alpha + n.beta
   where n is the total amount of data a process needs to send to all
   other processes.

   Possible improvements: 
   We could speculatively use a recursive doubling algorithm assuming
   a short message size and then switch to pairwise exchange if necessary.

   End Algorithm: MPI_Alltoallv
*/

PMPI_LOCAL int MPIR_Alltoallv ( 
	void *sendbuf, 
	int *sendcnts, 
	int *sdispls, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int *recvcnts, 
	int *rdispls, 
	MPI_Datatype recvtype, 
	MPID_Comm *comm_ptr )
{
    int        comm_size, i;
    MPI_Aint   send_extent, recv_extent;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank;
    MPI_Comm comm;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(sendtype, send_extent);
    MPID_Datatype_get_extent_macro(recvtype, recv_extent);
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );

    /* Use pairwise exchange algorithm. */
    
    /* Make local copy first */
    mpi_errno = MPIR_Localcopy(((char *)sendbuf+sdispls[rank]*send_extent), 
                               sendcnts[rank], sendtype, 
                               ((char *)recvbuf+rdispls[rank]*recv_extent), 
                               recvcnts[rank], recvtype);
    if (mpi_errno) return mpi_errno;
    /* Do the pairwise exchange. */
    for (i=1; i<comm_size; i++) {
        src = (rank - i + comm_size) % comm_size;
        dst = (rank + i) % comm_size;
        mpi_errno = MPIC_Sendrecv(((char *)sendbuf+sdispls[dst]*send_extent), 
                                  sendcnts[dst], sendtype, dst,
                                  MPIR_ALLTOALLV_TAG, 
                                  ((char *)recvbuf+rdispls[src]*recv_extent), 
                                  recvcnts[src], recvtype, src,
                                  MPIR_ALLTOALLV_TAG, comm, &status);
        if (mpi_errno) return mpi_errno;
    }
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alltoallv

/*@
   MPI_Alltoallv - alltoallv

   Arguments:
+  void *sendbuf - send buffer
.  int *sendcnts - send counts
.  int *sdispls - whatever
.  MPI_Datatype sendtype - send datatype
.  void *recvbuf - receive buffer
.  int *recvcnts - receive counts
.  int *rdispls - whatever
.  MPI_Datatype recvtype - receive datatype
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Alltoallv(void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype sendtype, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Alltoallv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLTOALLV);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLTOALLV);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
            int i, comm_size;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
            comm_size = comm_ptr->local_size;
            for (i=0; i<comm_size; i++) {
                MPIR_ERRTEST_COUNT(sendcnts[i], mpi_errno);
                MPIR_ERRTEST_COUNT(recvcnts[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendcnts[i], sendtype, mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvcnts[i], recvtype, mpi_errno);
	    }
            if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
                    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                }
            }
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
                    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Alltoallv != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Alltoallv(sendbuf, sendcnts, sdispls,
                                                 sendtype, recvbuf, recvcnts,
                                                 rdispls, recvtype, comm_ptr);
    }
    else
    {
	mpi_errno = MPIR_Alltoallv(sendbuf, sendcnts, sdispls,
                                   sendtype, recvbuf, recvcnts,
                                   rdispls, recvtype, comm_ptr);
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLV);
    return MPI_SUCCESS;
}
