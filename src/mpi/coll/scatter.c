/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Scatter = PMPI_Scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Scatter  MPI_Scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Scatter as PMPI_Scatter
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Scatter PMPI_Scatter

/* This is the default implementation of scatter. The algorithm is:
   
   Algorithm: MPI_Scatter

   We use a minimum spanning tree (MST) algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by 
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order. In the heterogeneous
   case, we first pack the buffer by using MPI_Pack and then do the
   scatter. 

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements: 

   End Algorithm: MPI_Scatter
*/

PMPI_LOCAL int MPIR_Scatter ( 
	void *sendbuf, 
	int sendcnt, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int recvcnt, 
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr )
{
    MPI_Status status;
    MPI_Aint   extent;
    int        rank, comm_size, is_homogeneous;
    int curr_cnt, relative_rank, nbytes, send_subtree_cnt;
    int mask, recvtype_size, src, dst, position, pack_size;
    void *tmp_buf=NULL;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Comm comm;
    
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        printf("ERROR: MPI_Scatter for intercommunicators not yet implemented.\n"); 
        NMPI_Abort(MPI_COMM_WORLD, 1);
    }

    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

/* Use MST algorithm */
    
    if (rank == root) 
        MPID_Datatype_get_extent_macro(sendtype, extent);
    
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );
    
    if (is_homogeneous) {
        /* communicator is homogeneous */
        
        MPID_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcnt;
        
        curr_cnt = 0;
        
        /* all even nodes other than root need a temporary buffer to
           receive data of max size (nbytes*comm_size)/2 */
        if (relative_rank && !(relative_rank % 2)) {
            tmp_buf = MPIU_Malloc((nbytes*comm_size)/2);
            if (!tmp_buf) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                return mpi_errno;
            }
        }
        
        /* if the root is not rank 0, we reorder the sendbuf in order of
           relative ranks and copy it into a temporary buffer, so that
           all the sends from the root are contiguous and in the right
           order. */
        
        if (rank == root) {
            if (root != 0) {
                tmp_buf = MPIU_Malloc(nbytes*comm_size);
                if (!tmp_buf) { 
                    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                    return mpi_errno;
                }

                position = 0;

                MPIR_Localcopy(((char *) sendbuf + extent*sendcnt*rank),
                               sendcnt*(comm_size-rank), sendtype, tmp_buf,
                               nbytes*(comm_size-rank), MPI_BYTE);
                MPIR_Localcopy(sendbuf, sendcnt*rank, sendtype, 
                               ((char *) tmp_buf + nbytes*(comm_size-rank)),
                               nbytes*rank, MPI_BYTE);

                /*
                NMPI_Pack(((char *) sendbuf + extent*sendcnt*rank),
                          sendcnt*(comm_size-rank), sendtype, tmp_buf,
                          nbytes*comm_size, &position, comm); 
                NMPI_Pack(sendbuf, sendcnt*rank, sendtype, tmp_buf,
                          nbytes*comm_size, &position, comm); 
                */
                curr_cnt = nbytes*comm_size;
            } 
            else 
                curr_cnt = sendcnt*comm_size;
        }
        
        /* root has all the data; others have zero so far */
        
        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask; 
                if (src < 0) src += comm_size;
                
                /* The leaf nodes receive directly into recvbuf because
                   they don't have to forward data to anyone. Others
                   receive data into a temporary buffer. */
                if (relative_rank % 2) {
                    mpi_errno = MPIC_Recv(recvbuf, recvcnt, recvtype,
                                          src, MPIR_SCATTER_TAG, comm, 
                                          &status);
                    if (mpi_errno) return mpi_errno;
                }
                else {
                    mpi_errno = MPIC_Recv(tmp_buf, mask * recvcnt *
                                          recvtype_size, MPI_BYTE, src,
                                          MPIR_SCATTER_TAG, comm, 
                                          &status);
                    if (mpi_errno) return mpi_errno;
                    
                    /* the recv size is larger than what may be sent in
                       some cases. query amount of data actually received */
                    NMPI_Get_count(&status, MPI_BYTE, &curr_cnt);
                }
                break;
            }
            mask <<= 1;
        }
        
        /* This process is responsible for all processes that have bits
           set from the LSB upto (but not including) mask.  Because of
           the "not including", we start by shifting mask back down
           one. */
        
        mask >>= 1;
        while (mask > 0) {
            if (relative_rank + mask < comm_size) {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                
                if ((rank == root) && (root == 0)) {
                    send_subtree_cnt = curr_cnt - sendcnt * mask; 
                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIC_Send (((char *)sendbuf + 
                                            extent * sendcnt * mask),
                                           send_subtree_cnt,
                                           sendtype, dst, 
                                           MPIR_SCATTER_TAG, comm);
                }
                else {
                    /* non-zero root and others */
                    send_subtree_cnt = curr_cnt - nbytes*mask; 
                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIC_Send (((char *)tmp_buf + nbytes*mask),
                                           send_subtree_cnt,
                                           MPI_BYTE, dst,
                                           MPIR_SCATTER_TAG, comm);
                }
                if (mpi_errno) return mpi_errno;
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
        
        if ((rank == root) && (root == 0)) {
            /* put root's data in the right place */
            mpi_errno = MPIR_Localcopy ( sendbuf, sendcnt, sendtype, 
                                         recvbuf, recvcnt, recvtype );
            if (mpi_errno) return mpi_errno;
        }
        else if (!(relative_rank % 2)) {
            /* for non-zero root and non-leaf nodes, copy from tmp_buf
               into recvbuf */ 
            mpi_errno = MPIR_Localcopy ( tmp_buf, nbytes, MPI_BYTE, 
                                         recvbuf, recvcnt, recvtype);
            if (mpi_errno) return mpi_errno;
            MPIU_Free(tmp_buf);
        }
    }
    
    else { /* communicator is heterogeneous */
    
        printf("ERROR: MPI_Scatter not implemented for heterogeneous case\n");
        NMPI_Abort(MPI_COMM_WORLD, 1);     
            
        if (rank == root) {
#ifdef UNIMPLEMENTED
            NMPI_Pack_size(sendcnt*comm_size, sendtype, comm,
                          &pack_size); 
#endif
            tmp_buf = MPIU_Malloc(pack_size);
            if (!tmp_buf) { 
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                return mpi_errno;
            }

            nbytes = pack_size/comm_size;
            curr_cnt = pack_size;
            
            position = 0;
#ifdef UNIMPLEMENTED
            if (root == 0)
                NMPI_Pack(sendbuf, sendcnt*comm_size, sendtype, tmp_buf,
                         pack_size, &position, comm);
            else {
                NMPI_Pack(((char *) sendbuf + extent*sendcnt*rank),
                         sendcnt*(comm_size-rank), sendtype, tmp_buf,
                         pack_size, &position, comm); 
                NMPI_Pack(sendbuf, sendcnt*rank, sendtype, tmp_buf,
                         pack_size, &position, comm); 
            }
#endif
        }
        else {
#ifdef UNIMPLEMENTED
            NMPI_Pack_size(recvcnt, recvtype, comm, &nbytes);
#endif
            tmp_buf = MPIU_Malloc((nbytes*comm_size)/2);
            if (!tmp_buf) { 
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                return mpi_errno;
            }
            curr_cnt = 0;
        }
        
        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask; 
                if (src < 0) src += comm_size;
                
                mpi_errno = MPIC_Recv(tmp_buf, mask*nbytes, MPI_BYTE, src,
                                     MPIR_SCATTER_TAG, comm, &status);
                if (mpi_errno) return mpi_errno;
                /* the recv size is larger than what may be sent in
                   some cases. query amount of data actually received */
                NMPI_Get_count(&status, MPI_BYTE, &curr_cnt);
                break;
            }
            mask <<= 1;
        }
        
        /* This process is responsible for all processes that have bits
           set from the LSB upto (but not including) mask.  Because of
           the "not including", we start by shifting mask back down
           one. */
        
        mask >>= 1;
        while (mask > 0) {
            if (relative_rank + mask < comm_size) {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                
                send_subtree_cnt = curr_cnt - nbytes * mask; 
                /* mask is also the size of this process's subtree */
                mpi_errno = MPIC_Send (((char *)tmp_buf + nbytes*mask),
                                      send_subtree_cnt, MPI_BYTE, dst,
                                      MPIR_SCATTER_TAG, comm);
                if (mpi_errno) return mpi_errno;
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
        
        /* copy local data into recvbuf */
        position = 0;
#ifdef UNIMPLEMENTED
        NMPI_Unpack(tmp_buf, nbytes, &position, recvbuf, recvcnt,
                   recvtype, comm);
#endif
        MPIU_Free(tmp_buf);
    }
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Scatter

/*@
   MPI_Scatter - scatter

   Arguments:
+  void *sendbuf - send buffer
.  int sendcnt - send count
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
int MPI_Scatter(void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Scatter";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SCATTER);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_SCATTER);

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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

            rank = comm_ptr->rank;
            if (rank == root) {
                MPIR_ERRTEST_COUNT(sendcnt, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendcnt, sendtype, mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) {
                        MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTER);
                        return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                    }
                }
            }

	    MPIR_ERRTEST_COUNT(recvcnt, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(recvcnt, recvtype, mpi_errno);
	    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
    
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTER);
                    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scatter != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Scatter(sendbuf, sendcnt,
                                                sendtype, recvbuf, recvcnt,
                                                recvtype, root, comm_ptr);
    }
    else
    {
	mpi_errno = MPIR_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt,
                                 recvtype, root, comm_ptr); 
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTER);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTER);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */

}
