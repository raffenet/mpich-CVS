/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Gather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Gather = PMPI_Gather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Gather  MPI_Gather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Gather as PMPI_Gather
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Gather PMPI_Gather
/* This is the default implementation of gather. The algorithm is:
   
   Algorithm: MPI_Gather

   We use a minimum spanning tree (MST) algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we receive data in a temporary buffer on the root and
   then reorder it into the right order. In the heterogeneous case
   we first pack the buffers by using MPI_Pack and then do the gather. 

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data gathered at the root.

   Possible improvements: 

   End Algorithm: MPI_Gather
*/

/* not declared static because it is called in intercomm. allgather */
int MPIR_Gather ( 
	void *sendbuf, 
	int sendcnt, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int recvcnt, 
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr )
{
    int        comm_size, rank;
    int        mpi_errno = MPI_SUCCESS;
    int curr_cnt=0, relative_rank, nbytes, recv_size, is_homogeneous;
    int mask, sendtype_size, src, dst, position, tmp_buf_size;
    void *tmp_buf=NULL;
    MPI_Status status;
    MPI_Aint   extent;            /* Datatype extent */
    MPI_Comm comm;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );
    
/* Use MST algorithm. */
    
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;
    
    if (rank == root) 
        MPID_Datatype_get_extent_macro(recvtype, extent);
    
    if (is_homogeneous) {
        /* communicator is homogeneous. no need to pack buffer. */
        
        MPID_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcnt;
        
        if (rank == root) {
            if (root != 0) {
                /* allocate temporary buffer to receive data because it
                   will not be in the right order. We will need to
                   reorder it into the recv_buf. */
                tmp_buf = MPIU_Malloc(nbytes*comm_size);
                if (!tmp_buf) {
                    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                    return mpi_errno;
                }

                if (sendbuf != MPI_IN_PLACE) {
                    /* copy root's sendbuf into tmpbuf just so that it is
                       easier to unpack everything later into the recv_buf */
                    mpi_errno = MPIR_Localcopy(sendbuf, sendcnt, sendtype,
                                               tmp_buf, nbytes, MPI_BYTE);
                    if (mpi_errno) return mpi_errno;
                }
                curr_cnt = nbytes;
            }
            else {
                /* root is 0. no tmp_buf needed at root. */
                /* copy root's sendbuf into recvbuf */
                if (sendbuf != MPI_IN_PLACE) {
                    mpi_errno = MPIR_Localcopy(sendbuf, sendcnt, sendtype,
                                               recvbuf, recvcnt, recvtype);
                    if (mpi_errno) return mpi_errno;
                }
                curr_cnt = recvcnt;
            }          
        }
        else if (!(relative_rank % 2)) {
            /* allocate temporary buffer for nodes other than leaf
               nodes. max size needed is (nbytes*comm_size)/2. */
            tmp_buf = MPIU_Malloc((nbytes*comm_size)/2);
            if (!tmp_buf) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                return mpi_errno;
            }
            /* copy from sendbuf into tmp_buf */
            mpi_errno = MPIR_Localcopy(sendbuf, sendcnt, sendtype,
                                       tmp_buf, nbytes, MPI_BYTE);
            if (mpi_errno) return mpi_errno;
            curr_cnt = nbytes;
        }
        
        mask = 0x1;
        while (mask < comm_size) {
            if ((mask & relative_rank) == 0) {
                src = relative_rank | mask;
                if (src < comm_size) {
                    src = (src + root) % comm_size;
                    if ((rank == root) && (root == 0)) {
                        /* root is 0. Receive directly into recvbuf */
                        mpi_errno = MPIC_Recv(((char *)recvbuf + 
                                               src*recvcnt*extent), 
                                              recvcnt*mask, recvtype, src,
                                              MPIR_GATHER_TAG, comm, 
                                              &status);
                        if (mpi_errno) return mpi_errno;
                    }
                    else {
                        /* intermediate nodes or nonzero root. store in
                           tmp_buf */
                        mpi_errno = MPIC_Recv(((char *)tmp_buf + curr_cnt), 
                                              mask*nbytes, MPI_BYTE, src,
                                              MPIR_GATHER_TAG, comm, 
                                              &status);
                        if (mpi_errno) return mpi_errno;
                        /* the recv size is larger than what may be sent in
                           some cases. query amount of data actually received */
                        NMPI_Get_count(&status, MPI_BYTE, &recv_size);
                        curr_cnt += recv_size;
                    }
                }
            }
            else {
                dst = relative_rank ^ mask;
                dst = (dst + root) % comm_size;
                if (relative_rank % 2) {
                    /* leaf nodes send directly from sendbuf */
                    mpi_errno = MPIC_Send(sendbuf, sendcnt, sendtype, dst,
                                          MPIR_GATHER_TAG, comm); 
                    if (mpi_errno) return mpi_errno;
                }
                else {
                    mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPI_BYTE, dst,
                                          MPIR_GATHER_TAG, comm); 
                    if (mpi_errno) return mpi_errno;
                }
                break;
            }
            mask <<= 1;
        }
        
        if ((rank == root) && (root != 0)) {
            /* reorder and copy from tmp_buf into recvbuf */
            position = 0;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_Localcopy(tmp_buf, nbytes*(comm_size-rank), MPI_BYTE, 
                               ((char *) recvbuf + extent*recvcnt*rank),
                               recvcnt*(comm_size-rank), recvtype);
            }
            else {
                MPIR_Localcopy((char *) tmp_buf + nbytes,
                               nbytes*(comm_size-rank-1), MPI_BYTE,  
                               ((char *) recvbuf + extent*recvcnt*(rank+1)),
                               recvcnt*(comm_size-rank-1), recvtype);
            }
            MPIR_Localcopy((char *) tmp_buf + nbytes*(comm_size-rank),
                           nbytes*rank, MPI_BYTE, recvbuf, 
                           recvcnt*rank, recvtype); 

            MPIU_Free(tmp_buf);
        }
        else if (relative_rank && !(relative_rank % 2))
            MPIU_Free(tmp_buf);
    }
    
    else { /* communicator is heterogeneous. pack data into tmp_buf. */

        printf("ERROR: MPI_Gather not implemented for heterogeneous case\n");
        NMPI_Abort(MPI_COMM_WORLD, 1);     
            
#ifdef UNIMPLEMENTED
        if (rank == root)
            NMPI_Pack_size(recvcnt*comm_size, recvtype, comm,
                           &tmp_buf_size); 
        else
            NMPI_Pack_size(sendcnt*(comm_size/2), sendtype, comm,
                           &tmp_buf_size);
#endif

        tmp_buf = MPIU_Malloc(tmp_buf_size);
        if (!tmp_buf) { 
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

        position = 0;
        if (sendbuf != MPI_IN_PLACE) {
#ifdef UNIMPLEMENTED
            NMPI_Pack(sendbuf, sendcnt, sendtype, tmp_buf,
                      tmp_buf_size, &position, comm);
#endif
            nbytes = position;
        }
        else {
            /* do a dummy pack just to calculate nbytes */
#ifdef UNIMPLEMENTED
            NMPI_Pack(recvbuf, 1, recvtype, tmp_buf,
                      tmp_buf_size, &position, comm);
#endif
            nbytes = position*recvcnt;
        }
        
        curr_cnt = nbytes;
        
        mask = 0x1;
        while (mask < comm_size) {
            if ((mask & relative_rank) == 0) {
                src = relative_rank | mask;
                if (src < comm_size) {
                    src = (src + root) % comm_size;
                    mpi_errno = MPIC_Recv(((char *)tmp_buf + curr_cnt), 
                                          mask*nbytes, MPI_BYTE, src,
                                          MPIR_GATHER_TAG, comm, 
                                          &status);
                    if (mpi_errno) return mpi_errno;
                    /* the recv size is larger than what may be sent in
                       some cases. query amount of data actually received */
                    NMPI_Get_count(&status, MPI_BYTE, &recv_size);
                    curr_cnt += recv_size;
                }
            }
            else {
                dst = relative_rank ^ mask;
                dst = (dst + root) % comm_size;
                mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPI_BYTE, dst,
                                      MPIR_GATHER_TAG, comm); 
                if (mpi_errno) return mpi_errno;
                break;
            }
            mask <<= 1;
        }
        
        if (rank == root) {
            /* reorder and copy from tmp_buf into recvbuf */
#ifdef UNIMPLEMENTED
            if (sendbuf != MPI_IN_PLACE) {
                position = 0;
                NMPI_Unpack(tmp_buf, tmp_buf_size, &position,
                            ((char *) recvbuf + extent*recvcnt*rank),
                            recvcnt*(comm_size-rank), recvtype, comm); 
            }
            else {
                position = nbytes;
                NMPI_Unpack(tmp_buf, tmp_buf_size, &position,
                            ((char *) recvbuf + extent*recvcnt*(rank+1)),
                            recvcnt*(comm_size-rank-1), recvtype,
                            comm);
            }
            if (root != 0)
                NMPI_Unpack(tmp_buf, tmp_buf_size, &position, recvbuf,
                            recvcnt*rank, recvtype, comm); 
#endif
        }
        
        MPIU_Free(tmp_buf);
    }

    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}


PMPI_LOCAL int MPIR_Gather_inter ( 
	void *sendbuf, 
	int sendcnt, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int recvcnt, 
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr )
{
/*  Intercommunicator gather.
    For short messages, remote group does a local intracommunicator
    gather to rank 0. Rank 0 then sends data to root.

    Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
   
    For long messages, we use linear gather to avoid the extra n.beta.

    Cost: p.alpha + n.beta
*/

    int rank, local_size, remote_size, mpi_errno=MPI_SUCCESS;
    int i, nbytes, sendtype_size, recvtype_size;
    MPI_Comm newcomm;
    MPI_Group group;
    MPI_Status status;
    MPI_Aint extent, lb=0;
    void *tmp_buf=NULL;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }
    
    comm = comm_ptr->handle;
    remote_size = comm_ptr->remote_size; 
    local_size = comm_ptr->local_size; 

    if (root == MPI_ROOT) {
        MPID_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcnt * remote_size;
    }
    else {
        /* remote side */
        MPID_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcnt * local_size;
    }

    if (nbytes < MPIR_GATHER_SHORT_MSG) {
        if (root == MPI_ROOT) {
            /* root receives data from rank 0 on remote group */
            MPID_Comm_thread_lock( comm_ptr );
            mpi_errno = MPIC_Recv(recvbuf, recvcnt*remote_size,
                                  recvtype, 0, MPIR_GATHER_TAG, comm,
                                  &status);
            MPID_Comm_thread_unlock( comm_ptr ); 
 
            return mpi_errno;
        }
        else {
            /* remote group. Rank 0 allocates temporary buffer, does
               local intracommunicator gather, and then sends the data
               to root. */
            
            rank = comm_ptr->rank;
            
            if (rank == 0) {
                MPID_Datatype_get_extent_macro(sendtype, extent);
                tmp_buf = MPIU_Malloc(extent*sendcnt*local_size);
                if (!tmp_buf) {
                    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                    return mpi_errno;
                }
                /* adjust for potential negative lower bound in datatype */
                /* MPI_Type_lb HAS NOT BEEN IMPLEMENTED YET. BUT lb IS
                   INITIALIZED TO 0, AND DERIVED DATATYPES AREN'T SUPPORTED YET,
                   SO IT'S OK */
#ifdef UNIMPLEMENTED
                MPI_Type_lb( sendtype, &lb );
#endif
                tmp_buf = (void *)((char*)tmp_buf - lb);
            }
            
            /* all processes in remote group form new intracommunicator */
#ifdef UNIMPLEMENTED
            NMPI_Comm_group(comm, &group);
            MPID_Comm_return_intra(group, &newcomm);
#endif
            MPID_Comm_get_ptr( newcomm, newcomm_ptr );
            /* now do the a local gather on this intracommunicator */
            mpi_errno = MPIR_Gather(sendbuf, sendcnt, sendtype,
                                    tmp_buf, sendcnt, sendtype, 0,
                                    newcomm_ptr); 
            if (rank == 0) {
                MPID_Comm_thread_lock( comm_ptr );
                mpi_errno = MPIC_Send(tmp_buf, sendcnt*local_size,
                                      sendtype, root,
                                      MPIR_GATHER_TAG, comm); 
                MPID_Comm_thread_unlock( comm_ptr ); 
                if (mpi_errno) return mpi_errno;
                MPIU_Free(tmp_buf+lb);
            }
        }
    }
    else {
        /* long message. use linear algorithm. */
        MPID_Comm_thread_lock( comm_ptr );
        if (root == MPI_ROOT) {
            MPID_Datatype_get_extent_macro(recvtype, extent);
            for (i=0; i<remote_size; i++) {
                mpi_errno = MPIC_Recv(((char *)recvbuf+recvcnt*i*extent), 
                                      recvcnt, recvtype, i,
                                      MPIR_GATHER_TAG, comm, &status);
                if (mpi_errno) return mpi_errno;                
            }
        }
        else {
            mpi_errno = MPIC_Send(sendbuf,sendcnt,sendtype,root,
                                  MPIR_GATHER_TAG,comm);
        }
        MPID_Comm_thread_unlock( comm_ptr ); 
    }

    return mpi_errno;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Gather

/*@
   MPI_Gather - gather

   Arguments:
+  void *sendbuf - send buffer
.  int sendcnt - send count
.  MPI_Datatype sendtype - send datatype
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
int MPI_Gather(void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Gather";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GATHER);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_GATHER);

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
	    int rank;

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHER);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }

            MPIR_ERRTEST_COUNT(sendcnt, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(sendcnt, sendtype, mpi_errno);
	    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

            rank = comm_ptr->rank;
            if (rank == root) {
                MPIR_ERRTEST_COUNT(recvcnt, mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvcnt, recvtype, mpi_errno);
                if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                    MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                }
            }
            if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
            }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Gather != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Gather(sendbuf, sendcnt,
                                               sendtype, recvbuf, recvcnt,
                                               recvtype, root, comm_ptr);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Gather(sendbuf, sendcnt, sendtype,
                                    recvbuf, recvcnt, recvtype, root,
                                    comm_ptr);  
        else {
            /* intercommunicator */ 
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );
            /*mpi_errno = MPIR_Gather_inter(sendbuf, sendcnt, sendtype,
                                          recvbuf, recvcnt, recvtype, root,
                                          comm_ptr);*/
        }
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHER);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHER);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */

}
