/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Allgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allgather = PMPI_Allgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allgather  MPI_Allgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allgather as PMPI_Allgather
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Allgather PMPI_Allgather

/* This is the default implementation of allgather. The algorithm is:
   
   Algorithm: MPI_Allgather

   For both short and long messages, we use a recursive doubling
   algorithm, which takes lgp steps. In each step, pairs of processes
   exchange all the data they have. We take care of non-power-of-two
   situations. 
   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.
   (The cost may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) 

   It is interesting to note that this algorithm for MPI_Allgather has
   the same cost as the binary tree algorithm for MPI_Gather!

   Possible improvements: 

   End Algorithm: MPI_Allgather
*/
/* begin:nested */
PMPI_LOCAL int MPIR_Allgather ( 
    void *sendbuf, 
    int sendcount, 
    MPI_Datatype sendtype,
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr )
{
    static const char FCNAME[] = "MPIR_Allgather";
    int        comm_size, rank;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint   recv_extent;
    int        j, i, is_homogeneous, tmp_buf_size;
    int curr_cnt, mask, dst, dst_tree_root, my_tree_root, 
        send_offset, recv_offset, last_recv_cnt, nprocs_completed, k,
        offset, tmp_mask, tree_root, position, nbytes;
    void *tmp_buf;
    MPI_Comm comm;

    if (sendcount == 0) return MPI_SUCCESS;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    
    MPID_Datatype_get_extent_macro( recvtype, recv_extent );

    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );
    
    /* use recursive doubling algorithm */
    
    if (is_homogeneous) {
        /* homogeneous. no need to pack into tmp_buf on each node. copy
           local data into recvbuf */ 
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy (sendbuf, sendcount, sendtype,
                                        ((char *)recvbuf +
                                         rank*recvcount*recv_extent), 
                                        recvcount, recvtype);
            if (mpi_errno) return mpi_errno;
        }

        curr_cnt = recvcount;
        
        mask = 0x1;
        i = 0;
        while (mask < comm_size) {
            dst = rank ^ mask;
            
            /* find offset into send and recv buffers. zero out 
               the least significant "i" bits of rank and dst to 
               find root of src and dst subtrees. Use ranks of 
               roots as index to send from and recv into buffer */ 
            
            dst_tree_root = dst >> i;
            dst_tree_root <<= i;
            
            my_tree_root = rank >> i;
            my_tree_root <<= i;
            
            send_offset = my_tree_root * recvcount * recv_extent;
            recv_offset = dst_tree_root * recvcount * recv_extent;
            
            if (dst < comm_size) {
                mpi_errno = MPIC_Sendrecv(((char *)recvbuf + send_offset),
                                         curr_cnt, recvtype, dst,
                                          MPIR_ALLGATHER_TAG,  
                                         ((char *)recvbuf + recv_offset),
                                         recvcount*mask, recvtype, dst,
                                         MPIR_ALLGATHER_TAG, comm, &status);
                if (mpi_errno) return mpi_errno;
                
                NMPI_Get_count(&status, recvtype, &last_recv_cnt);
                curr_cnt += last_recv_cnt;
            }
            
            /* if some processes in this process's subtree in this step
               did not have any destination process to communicate with
               because of non-power-of-two, we need to send them the
               data that they would normally have received from those
               processes. That is, the haves in this subtree must send to
               the havenots. We use a logarithmic recursive-halfing algorithm
               for this. */
            
            if (dst_tree_root + mask > comm_size) {
                nprocs_completed = comm_size - my_tree_root - mask;
                /* nprocs_completed is the number of processes in this
                   subtree that have all the data. Send data to others
                   in a tree fashion. First find root of current tree
                   that is being divided into two. k is the number of
                   least-significant bits in this process's rank that
                   must be zeroed out to find the rank of the root */ 
                j = mask;
                k = 0;
                while (j) {
                    j >>= 1;
                    k++;
                }
                k--;
                
                offset = recvcount * (my_tree_root + mask) * recv_extent;
                tmp_mask = mask >> 1;
                
                while (tmp_mask) {
                    dst = rank ^ tmp_mask;
                    
                    tree_root = rank >> k;
                    tree_root <<= k;
                    
                    /* send only if this proc has data and destination
                       doesn't have data. at any step, multiple processes
                       can send if they have the data */
                    if ((dst > rank) && 
                        (rank < tree_root + nprocs_completed)
                        && (dst >= tree_root + nprocs_completed)) {
                        mpi_errno = MPIC_Send(((char *)recvbuf + offset),
                                             last_recv_cnt,
                                             recvtype, dst,
                                             MPIR_ALLGATHER_TAG, comm); 
                        /* last_recv_cnt was set in the previous
                           receive. that's the amount of data to be
                           sent now. */
                        if (mpi_errno) return mpi_errno;
                    }
                    /* recv only if this proc. doesn't have data and sender
                       has data */
                    else if ((dst < rank) && 
                             (dst < tree_root + nprocs_completed) &&
                             (rank >= tree_root + nprocs_completed)) {
                        mpi_errno = MPIC_Recv(((char *)recvbuf + offset),  
                                              recvcount*nprocs_completed, 
                                              recvtype, dst,
                                              MPIR_ALLGATHER_TAG,
                                              comm, &status); 
                        /* nprocs_completed is also equal to the
                           no. of processes whose data we don't have */
                        if (mpi_errno) return mpi_errno;
                        NMPI_Get_count(&status, recvtype, &last_recv_cnt);
                        curr_cnt += last_recv_cnt;
                    }
                    tmp_mask >>= 1;
                    k--;
                }
            }
            
            mask <<= 1;
            i++;
        }
    }
    
    else {
        /* heterogeneous. need to use temp. buffer. */
 
        NMPI_Pack_size(recvcount*comm_size, recvtype, comm, &tmp_buf_size);

        tmp_buf = MPIU_Malloc(tmp_buf_size);
        if (!tmp_buf) { 
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

      /* calculate the value of nbytes, the number of bytes in packed
         representation that each process contributes. We can't simply divide
         tmp_buf_size by comm_size because tmp_buf_size is an upper
         bound on the amount of memory required. (For example, for
         a single integer, MPICH-1 returns pack_size=12.) Therefore, we
         actually pack some data into tmp_buf and see by how much
         'position' is incremented. */

        position = 0;
        NMPI_Pack(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size,
                  &position, comm);
        nbytes = position*recvcount;

        /* pack local data into right location in tmp_buf */
        position = rank * nbytes;
        if (sendbuf != MPI_IN_PLACE) {
            NMPI_Pack(sendbuf, sendcount, sendtype, tmp_buf, tmp_buf_size,
                      &position, comm);
        }
        else {
            /* if in_place specified, local data is found in recvbuf */
            NMPI_Pack(((char *)recvbuf + recv_extent*rank), recvcount,
                       recvtype, tmp_buf, tmp_buf_size, 
                       &position, comm);
        }

        curr_cnt = nbytes;
        
        mask = 0x1;
        i = 0;
        while (mask < comm_size) {
            dst = rank ^ mask;
            
            /* find offset into send and recv buffers. zero out 
               the least significant "i" bits of rank and dst to 
               find root of src and dst subtrees. Use ranks of 
               roots as index to send from and recv into buffer. */ 
            
            dst_tree_root = dst >> i;
            dst_tree_root <<= i;
            
            my_tree_root = rank >> i;
            my_tree_root <<= i;
            
            send_offset = my_tree_root * nbytes;
            recv_offset = dst_tree_root * nbytes;
            
            if (dst < comm_size) {
                mpi_errno = MPIC_Sendrecv(((char *)tmp_buf + send_offset),
                                          curr_cnt, MPI_BYTE, dst,
                                          MPIR_ALLGATHER_TAG,  
                                          ((char *)tmp_buf + recv_offset),
                                          nbytes*mask, MPI_BYTE, dst,
                                          MPIR_ALLGATHER_TAG, comm, &status);
                if (mpi_errno) return mpi_errno;
                
                NMPI_Get_count(&status, MPI_BYTE, &last_recv_cnt);
                curr_cnt += last_recv_cnt;
            }
            
            /* if some processes in this process's subtree in this step
               did not have any destination process to communicate with
               because of non-power-of-two, we need to send them the
               data that they would normally have received from those
               processes. That is, the haves in this subtree must send to
               the havenots. We use a logarithmic recursive-halfing algorithm
               for this. */
            
            if (dst_tree_root + mask > comm_size) {
                nprocs_completed = comm_size - my_tree_root - mask;
                /* nprocs_completed is the number of processes in this
                   subtree that have all the data. Send data to others
                   in a tree fashion. First find root of current tree
                   that is being divided into two. k is the number of
                   least-significant bits in this process's rank that
                   must be zeroed out to find the rank of the root */ 
                j = mask;
                k = 0;
                while (j) {
                    j >>= 1;
                    k++;
                }
                k--;
                
                offset = nbytes * (my_tree_root + mask);
                tmp_mask = mask >> 1;
                
                while (tmp_mask) {
                    dst = rank ^ tmp_mask;
                    
                    tree_root = rank >> k;
                    tree_root <<= k;
                    
                    /* send only if this proc has data and destination
                       doesn't have data. at any step, multiple processes
                       can send if they have the data */
                    if ((dst > rank) && 
                        (rank < tree_root + nprocs_completed)
                        && (dst >= tree_root + nprocs_completed)) {
                        
                        mpi_errno = MPIC_Send(((char *)tmp_buf + offset),
                                             last_recv_cnt, MPI_BYTE,
                                             dst, MPIR_ALLGATHER_TAG,
                                             comm);  
                        /* last_recv_cnt was set in the previous
                           receive. that's the amount of data to be
                           sent now. */
                        if (mpi_errno) return mpi_errno;
                    }
                    /* recv only if this proc. doesn't have data and sender
                       has data */
                    else if ((dst < rank) && 
                             (dst < tree_root + nprocs_completed) &&
                             (rank >= tree_root + nprocs_completed)) {
                        mpi_errno = MPIC_Recv(((char *)tmp_buf + offset),
                                              nbytes*nprocs_completed,
                                              MPI_BYTE, dst,
                                              MPIR_ALLGATHER_TAG,
                                              comm, &status); 
                        /* nprocs_completed is also equal to the
                           no. of processes whose data we don't have */
                        if (mpi_errno) return mpi_errno;
                        NMPI_Get_count(&status, MPI_BYTE, &last_recv_cnt);
                        curr_cnt += last_recv_cnt;
                    }
                    tmp_mask >>= 1;
                    k--;
                }
            }
            mask <<= 1;
            i++;
        }
        
        position = 0;
        NMPI_Unpack(tmp_buf, tmp_buf_size, &position, recvbuf,
                    recvcount*comm_size, recvtype, comm);
        
        MPIU_Free(tmp_buf);
    }
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    return (mpi_errno);
}
/* end:nested */

/* begin:nested */
PMPI_LOCAL int MPIR_Allgather_inter ( 
    void *sendbuf, 
    int sendcount, 
    MPI_Datatype sendtype,
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr )
{
/* Intercommunicator Allgather.
   Each group does a gather to local root with the local
   intracommunicator, and then does an intercommunicator broadcast.
*/

    static const char FCNAME[] = "MPIR_Allgather_inter";
    int rank, local_size, remote_size, mpi_errno, root;
    MPI_Comm newcomm;
    MPI_Aint true_extent, true_lb, extent, send_extent;
    void *tmp_buf=NULL;
    MPID_Comm *newcomm_ptr = NULL;

    local_size = comm_ptr->local_size; 
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    if (rank == 0) {
        /* In each group, rank 0 allocates temp. buffer for local
           gather */
        mpi_errno = NMPI_Type_get_true_extent(sendtype, &true_lb,
                                              &true_extent);  
        if (mpi_errno) return mpi_errno;
        MPID_Datatype_get_extent_macro( sendtype, send_extent );
        extent = MPIR_MAX(send_extent, true_extent);

        tmp_buf = MPIU_Malloc(extent*sendcount*local_size);
        if (!tmp_buf) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
	MPIR_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;
    newcomm = newcomm_ptr->handle;

    mpi_errno = MPIR_Gather(sendbuf, sendcount, sendtype, tmp_buf, sendcount,
                            sendtype, 0, newcomm_ptr);
    if (mpi_errno) return mpi_errno;

    /* first broadcast from left to right group, then from right to
       left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast(tmp_buf, sendcount*local_size,
                               sendtype, root, comm_ptr); 
        if (mpi_errno) return mpi_errno;
        /* receive bcast from right */
        root = 0;
        mpi_errno = MPIR_Bcast(recvbuf, recvcount*remote_size,
                               recvtype, root, comm_ptr); 
        if (mpi_errno) return mpi_errno;
    }
    else {
        /* receive bcast from left */
        root = 0;
        mpi_errno = MPIR_Bcast(recvbuf, recvcount*remote_size,
                               recvtype, root, comm_ptr); 
        if (mpi_errno) return mpi_errno;
        /* bcast to left */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Bcast(tmp_buf, sendcount*local_size,
                               sendtype, root, comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }
    
    if (rank == 0)
        MPIU_Free((char*)tmp_buf+true_lb);

    return mpi_errno;
}
/* end:nested */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allgather

/*@
MPI_Allgather - Gathers data from all tasks and distribute it to all 

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcount - number of elements in send buffer (integer) 
. sendtype - data type of send buffer elements (handle) 
. recvcount - number of elements received from any process (integer) 
. recvtype - data type of receive buffer elements (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - address of receive buffer (choice) 

Notes:
 The MPI standard (1.0 and 1.1) says that 

 The jth block of data sent from 
 each proess is received by every process and placed in the jth block of the 
 buffer 'recvbuf'.  

 This is misleading; a better description is

 The block of data sent from the jth process is received by every
 process and placed in the jth block of the buffer 'recvbuf'.

 This text was suggested by Rajeev Thakur and has been adopted as a 
 clarification.

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Allgather";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLGATHER);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLGATHER);

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
            MPID_Datatype *recvtype_ptr=NULL, *sendtype_ptr=NULL;

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHER);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }

            MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
	    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(sendcount, sendtype, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(recvcount, recvtype, mpi_errno);
            if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
            }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Allgather != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Allgather(sendbuf, sendcount,
                                                  sendtype, recvbuf, recvcount,
                                                  recvtype, comm_ptr);
    }
    else
    {
	MPIR_Nest_incr();
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Allgather(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype,
                                       comm_ptr);
        else {
            /* intercommunicator */
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );
            /*mpi_errno = MPIR_Allgather_inter(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             comm_ptr);            */
        }
	MPIR_Nest_decr();
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHER);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHER);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
