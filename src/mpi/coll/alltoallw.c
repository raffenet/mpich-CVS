/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Alltoallw */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoallw = PMPI_Alltoallw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoallw  MPI_Alltoallw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoallw as PMPI_Alltoallw
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Alltoallw PMPI_Alltoallw
/* This is the default implementation of alltoallw. The algorithm is:
   
   Algorithm: MPI_Alltoallw

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

   End Algorithm: MPI_Alltoallw
*/

PMPI_LOCAL int MPIR_Alltoallw ( 
	void *sendbuf, 
	int *sendcnts, 
	int *sdispls, 
	MPI_Datatype *sendtypes, 
	void *recvbuf, 
	int *recvcnts, 
	int *rdispls, 
	MPI_Datatype *recvtypes, 
	MPID_Comm *comm_ptr )
{
    int        comm_size, i;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank;
    MPI_Comm comm;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );

    /* Use pairwise exchange algorithm. */
    
    /* Make local copy first */
    mpi_errno = MPIR_Localcopy(((char *)sendbuf+sdispls[rank]), 
                               sendcnts[rank], sendtypes[rank], 
                               ((char *)recvbuf+rdispls[rank]), 
                               recvcnts[rank], recvtypes[rank]);
    if (mpi_errno) return mpi_errno;
    /* Do the pairwise exchange. */
    for (i=1; i<comm_size; i++) {
        src = (rank - i + comm_size) % comm_size;
        dst = (rank + i) % comm_size;
        mpi_errno = MPIC_Sendrecv(((char *)sendbuf+sdispls[dst]), 
                                  sendcnts[dst], sendtypes[dst], dst,
                                  MPIR_ALLTOALLW_TAG, 
                                  ((char *)recvbuf+rdispls[src]), 
                                  recvcnts[src], recvtypes[dst], src,
                                  MPIR_ALLTOALLW_TAG, comm, &status);
        if (mpi_errno) return mpi_errno;
    }
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}


PMPI_LOCAL int MPIR_Alltoallw_inter ( 
	void *sendbuf, 
	int *sendcnts, 
	int *sdispls, 
	MPI_Datatype *sendtypes, 
	void *recvbuf, 
	int *recvcnts, 
	int *rdispls, 
	MPI_Datatype *recvtypes, 
	MPID_Comm *comm_ptr )
{
/* Intercommunicator alltoallw. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoallw. Since the
   local and remote groups can be of different 
   sizes, we first compute the max of local_group_size,
   remote_group_size. At step i, 0 <= i < max_size, each process
   receives from src = (rank - i + max_size) % max_size if src <
   remote_size, and sends to dst = (rank + i) % max_size if dst <
   remote_size. 
*/

    int local_size, remote_size, max_size, i;
    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;
    MPI_Datatype sendtype, recvtype;
    MPI_Comm comm;
    
    local_size = comm_ptr->local_size; 
    remote_size = comm_ptr->remote_size;
    comm = comm_ptr->handle;
    rank = comm_ptr->rank;

    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );

    /* Use pairwise exchange algorithm. */
    max_size = MPIR_MAX(local_size, remote_size);
    for (i=0; i<max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
            recvcount = 0;
            recvtype = MPI_DATATYPE_NULL;
        }
        else {
            recvaddr = (char *)recvbuf + rdispls[src];
            recvcount = recvcnts[src];
            recvtype = recvtypes[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
            sendtype = MPI_DATATYPE_NULL;
        }
        else {
            sendaddr = (char *)sendbuf+sdispls[dst];
            sendcount = sendcnts[dst];
            sendtype = sendtypes[dst];
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype, 
                                  dst, MPIR_ALLTOALLW_TAG, recvaddr, 
                                  recvcount, recvtype, src,
                                  MPIR_ALLTOALLW_TAG, comm, &status);
        if (mpi_errno) return mpi_errno;
    }
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alltoallw

/*@
   MPI_Alltoallw - alltoallw

   Arguments:
+  void *sendbuf - send buffer
.  int *sendcnts - send counts
.  int *sdispls - send displacements
.  MPI_Datatype *sendtypes - send datatypes
.  void *recvbuf - receive buffer
.  int *recvcnts - receive counts
.  int *rdispls - receive displacements
.  MPI_Datatype *recvtypes - receive datatypes
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Alltoallw(void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Alltoallw";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLTOALLW);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLTOALLW);

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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLW);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
            comm_size = comm_ptr->local_size;
            for (i=0; i<comm_size; i++) {
                MPIR_ERRTEST_COUNT(sendcnts[i], mpi_errno);
                MPIR_ERRTEST_COUNT(recvcnts[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendcnts[i], sendtypes[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvcnts[i], recvtypes[i], mpi_errno);

                if (HANDLE_GET_KIND(sendtypes[i]) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(sendtypes[i], sendtype_ptr);
                    MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                }
                if (HANDLE_GET_KIND(recvtypes[i]) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(recvtypes[i], recvtype_ptr);
                    MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                }
            }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLW);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Alltoallw != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Alltoallw(sendbuf, sendcnts, sdispls,
                                                 sendtypes, recvbuf, recvcnts,
                                                 rdispls, recvtypes, comm_ptr);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Alltoallw(sendbuf, sendcnts, sdispls,
                                       sendtypes, recvbuf, recvcnts,
                                       rdispls, recvtypes, comm_ptr);
        else {
            /* intercommunicator */
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );
            /*mpi_errno = MPIR_Alltoallw_inter(sendbuf, sendcnts, sdispls,
                                       sendtypes, recvbuf, recvcnts,
                                       rdispls, recvtypes, comm_ptr);*/
        }
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLW);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLW);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALLW);
    return MPI_SUCCESS;
}
