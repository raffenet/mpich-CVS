/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Gatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Gatherv = PMPI_Gatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Gatherv  MPI_Gatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Gatherv as PMPI_Gatherv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Gatherv PMPI_Gatherv

/* This is the default implementation of gatherv. The algorithm is:
   
   Algorithm: MPI_Gatherv

   Since the array of recvcounts is valid only on the root, we cannot
   do a tree algorithm without first communicating the recvcounts to
   other processes. Therefore, we simply use a linear algorithm for the
   gather, which takes (p-1) steps versus lgp steps for the tree
   algorithm. The bandwidth requirement is the same for both algorithms.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements: 

   End Algorithm: MPI_Gatherv
*/

PMPI_LOCAL int MPIR_Gatherv ( 
	void *sendbuf, 
	int sendcnt,  
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int *recvcnts, 
	int *displs, 
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr )
{
    int        comm_size, rank;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Comm comm;
    
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        printf("ERROR: MPI_Gatherv for intercommunicators not yet implemented.\n"); 
        NMPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );
    
    /* If rank == root, then I recv lots, otherwise I send */
    if ( rank == root ) {
        MPI_Aint       extent;
        int            i;
	MPI_Status       status;
        
        MPID_Datatype_get_extent_macro(recvtype, extent);
        for ( i=0; i<root; i++ ) {
            mpi_errno = MPIC_Recv(((char *)recvbuf+displs[i]*extent), 
                                 recvcnts[i], recvtype, i,
                                 MPIR_GATHERV_TAG, comm, &status);
            if (mpi_errno) return mpi_errno;
        }
        mpi_errno = MPIR_Localcopy(sendbuf, sendcnt, sendtype,
                                   ((char *)recvbuf+displs[rank]*extent), 
                                   recvcnts[rank], recvtype);
        if (mpi_errno) return mpi_errno;
        for ( i=root+1; i<comm_size; i++ ) {
            mpi_errno = MPIC_Recv(((char *)recvbuf+displs[i]*extent), 
                                 recvcnts[i], recvtype, i,
                                 MPIR_GATHERV_TAG, comm, &status);
            if (mpi_errno) return mpi_errno;
        }
    }
    else 
        mpi_errno = MPIC_Send(sendbuf, sendcnt, sendtype, root, 
                              MPIR_GATHERV_TAG, comm);
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Gatherv

/*@
   MPI_Gatherv - gatherv

   Arguments:
+  void *sendbuf - send buffer
.  int sendcnt - send count
.  MPI_Datatype sendtype - send datatype
.  void *recvbuf - receive buffer
.  int *recvcnts - receive counts
.  int *displs - whatever
.  MPI_Datatype recvtype - receive datatype
.  int root - root
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Gatherv(void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int *recvcnts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Gatherv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GATHERV);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_GATHERV);

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
            int i, rank, comm_size;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHERV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

            MPIR_ERRTEST_COUNT(sendcnt, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(sendcnt, sendtype, mpi_errno);
	    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

            rank = comm_ptr->rank;
            if (rank == root) {
                comm_size = comm_ptr->local_size;
                for (i=0; i<comm_size; i++) {
                    MPIR_ERRTEST_COUNT(recvcnts[i], mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvcnts[i], recvtype, mpi_errno);
                }
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHERV);
                    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                }
            }
    
	    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
            MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHERV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Gather != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Gatherv(sendbuf, sendcnt,
                                                sendtype, recvbuf, recvcnts,
                                                displs, recvtype, root,
                                                comm_ptr);  
    }
    else
    {
	mpi_errno = MPIR_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts,
                                 displs, recvtype, root, comm_ptr); 
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHERV);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHERV);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
