/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Scatterv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Scatterv = PMPI_Scatterv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Scatterv  MPI_Scatterv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Scatterv as PMPI_Scatterv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Scatterv PMPI_Scatterv

/* This is the default implementation of scatterv. The algorithm is:
   
   Algorithm: MPI_Scatterv

   Since the array of sendcounts is valid only on the root, we cannot
   do a tree algorithm without first communicating the sendcounts to
   other processes. Therefore, we simply use a linear algorithm for the
   scatter, which takes (p-1) steps versus lgp steps for the tree
   algorithm. The bandwidth requirement is the same for both algorithms.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements: 

   End Algorithm: MPI_Scatterv
*/

/* not declared static because it is called in intercomm. reduce_scatter */
int MPIR_Scatterv ( 
	void *sendbuf, 
	int *sendcnts, 
	int *displs, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int recvcnt,  
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr )
{
    MPI_Status status;
    int        rank, comm_size, remote_comm_size;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Comm comm;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );
    
    /* If I'm the root, then scatter */
    if ( rank == root ) {
        MPI_Aint extent;
        int      i;
        
        MPID_Datatype_get_extent_macro(sendtype, extent);
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
            /* We could use Isend here, but since the receivers need to execute
               a simple Recv, it may not make much difference in performance, 
               and using the blocking version is simpler */
            for ( i=0; i<root; i++ ) {
                mpi_errno = MPIC_Send(((char *)sendbuf+displs[i]*extent), 
                                      sendcnts[i], sendtype, i,
                                      MPIR_SCATTERV_TAG, comm);
                if (mpi_errno) return mpi_errno;
            }
            if (recvbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Localcopy(((char *)sendbuf+displs[rank]*extent), 
                                           sendcnts[rank], sendtype, 
                                           recvbuf, recvcnt, recvtype);
                if (mpi_errno) return mpi_errno;
            }        
            for ( i=root+1; i<comm_size; i++ ) {
                mpi_errno = MPIC_Send(((char *)sendbuf+displs[i]*extent), 
                                      sendcnts[i], sendtype, i, 
                                      MPIR_SCATTERV_TAG, comm);
                if (mpi_errno) return mpi_errno;
            }
        }
        else {
            /* intercommunicator */
            remote_comm_size = comm_ptr->remote_size;
            for (i=0; i<remote_comm_size; i++) {
                mpi_errno = MPIC_Send(((char *)sendbuf+displs[i]*extent), 
                                      sendcnts[i], sendtype, i,
                                      MPIR_SCATTERV_TAG, comm);
                if (mpi_errno) return mpi_errno;                
            }
        }
    }
    else
        mpi_errno = MPIC_Recv(recvbuf,recvcnt,recvtype,root,
                              MPIR_SCATTERV_TAG,comm,&status);
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Scatterv

/*@
   MPI_Scatterv - scatterv

   Arguments:
+   void *sendbuf - send buffer
.  int *sendcnts - send counts
.  int *displs - send displacements
.  MPI_Datatype sendtype - send type
.  void *recvbuf - receive buffer
.  int recvcnt - receive count
.  MPI_Datatype recvtype - receive datatype
.  int root - root
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Scatterv( void *sendbuf, int *sendcnts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcnt,  MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Scatterv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SCATTERV);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_SCATTERV);

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
            int i, comm_size, rank;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTERV);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }

            rank = comm_ptr->rank;
            if (rank == root) {
                comm_size = comm_ptr->local_size;
                for (i=0; i<comm_size; i++) {
                    MPIR_ERRTEST_COUNT(sendcnts[i], mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendcnts[i], sendtype, mpi_errno);
                }
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                }
            }

	    MPIR_ERRTEST_COUNT(recvcnt, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(recvcnt, recvtype, mpi_errno);
	    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
    
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
            }

            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTERV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scatter != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Scatterv(sendbuf, sendcnts, displs,
                                                sendtype, recvbuf, recvcnt,
                                                recvtype, root, comm_ptr);
    }
    else
    {
	mpi_errno = MPIR_Scatterv(sendbuf, sendcnts, displs, sendtype, 
                                  recvbuf, recvcnt, recvtype, root, comm_ptr); 
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTERV);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTERV);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */

}
 
