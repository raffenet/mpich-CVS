/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Allgatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allgatherv = PMPI_Allgatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allgatherv  MPI_Allgatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allgatherv as PMPI_Allgatherv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Allgatherv PMPI_Allgatherv

/* This is the default implementation of allgatherv. The algorithm is:
   
   Algorithm: MPI_Allgatherv

   For short and medium-size messages (up to 256KB), we use a
   recursive doubling algorithm, which takes lgp steps. In each step,
   pairs of processes exchange all the data they have. We take care of
   non-power-of-two situations. 
   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.
   (The cost may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) 

   Because of displacements in the recvbuf, the data received may be
   noncontiguously located. In the recursive doubling algorithm, this
   requires us to use a temp buffer, collect all the data
   contiguously, and then copy it into the recvbuf. 

   For long messages, to avoid the overhead of copying data, we
   instead use the ring algorithm where each process sends its own
   contribution around the ring of processes. This takes p-1 steps
   but has the same bandwidth requirement as recursive doubling.
   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements: 

   End Algorithm: MPI_Allgatherv
*/

/* begin:nested */
PMPI_LOCAL int MPIR_Allgatherv ( 
    void *sendbuf, 
    int sendcount,   
    MPI_Datatype sendtype, 
    void *recvbuf, 
    int *recvcounts, 
    int *displs,   
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr )
{
    static const char FCNAME[] = "MPIR_Allgatherv";
    MPI_Comm comm;
    int        comm_size, rank, j, i, jnext, left, right;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint   recv_extent, true_extent, true_lb;
    int curr_cnt, mask, dst, dst_tree_root, my_tree_root, is_homogeneous,
        send_offset, recv_offset, last_recv_cnt, nprocs_completed, k,
        offset, tmp_mask, tree_root, position, total_count,
        type_size; 
#ifdef MPID_HAS_HETERO
    int tmp_buf_size, nbytes;
#endif
    void *tmp_buf;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0) return MPI_SUCCESS;
    
    MPID_Datatype_get_extent_macro( recvtype, recv_extent );
    MPID_Datatype_get_size_macro(recvtype, type_size);
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    if (total_count*type_size > MPIR_ALLGATHERV_MEDIUM_MSG) {
        /* long message. use ring algorithm. */

        if (sendbuf != MPI_IN_PLACE) {
            /* First, load the "local" version in the recvbuf. */
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, 
                                       ((char *)recvbuf + displs[rank]*recv_extent),
                                       recvcounts[rank], recvtype);
            if (mpi_errno) return mpi_errno;
        }

        left  = (comm_size + rank - 1) % comm_size;
        right = (rank + 1) % comm_size;
  
        j     = rank;
        jnext = left;
        for (i=1; i<comm_size; i++) {
            mpi_errno = MPIC_Sendrecv(((char *)recvbuf+displs[j]*recv_extent),
                                      recvcounts[j], recvtype, right,
                                      MPIR_ALLGATHERV_TAG, 
                                 ((char *)recvbuf + displs[jnext]*recv_extent),
                                      recvcounts[jnext], recvtype, left, 
                                      MPIR_ALLGATHERV_TAG, comm, &status );
            if (mpi_errno) return mpi_errno;
            j	    = jnext;
            jnext = (comm_size + jnext - 1) % comm_size;
        }
    }

    else { /* Short or medium size message. Use recursive doubling algorithm */
    
        is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
        if (comm_ptr->is_hetero)
            is_homogeneous = 0;
#endif
        
        if (is_homogeneous) {
            /* need to receive contiguously into tmp_buf because
               displs could make the recvbuf noncontiguous */

            mpi_errno = NMPI_Type_get_true_extent(recvtype, &true_lb,
                                                  &true_extent);  
            if (mpi_errno) return mpi_errno;
            
            tmp_buf =
                MPIU_Malloc(total_count*(MPIR_MAX(true_extent,recv_extent)));  
            if (!tmp_buf) { 
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                return mpi_errno;
            }

            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);

            /* copy local data into right location in tmp_buf */ 
            position = 0;
            for (i=0; i<rank; i++) position += recvcounts[i];
            if (sendbuf != MPI_IN_PLACE)
                mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                           ((char *)tmp_buf + position*
                                            recv_extent), 
                                           recvcounts[rank], recvtype);
            else 
                /* if in_place specified, local data is found in recvbuf */ 
                mpi_errno = MPIR_Localcopy(((char *)recvbuf +
                                            displs[rank]*recv_extent), 
                                           recvcounts[rank], recvtype,
                                           ((char *)tmp_buf + position*
                                            recv_extent), 
                                           recvcounts[rank], recvtype);
            if (mpi_errno) return mpi_errno;

            curr_cnt = recvcounts[rank];
            
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
                
                if (dst < comm_size) {
                    send_offset = 0;
                    for (j=0; j<my_tree_root; j++)
                        send_offset += recvcounts[j];
                    send_offset *= recv_extent;
                    
                    recv_offset = 0;
                    for (j=0; j<dst_tree_root; j++)
                        recv_offset += recvcounts[j];
                    recv_offset *= recv_extent;

                    mpi_errno = MPIC_Sendrecv(((char *)tmp_buf + send_offset),
                                              curr_cnt, recvtype, dst,
                                              MPIR_ALLGATHERV_TAG,  
                                              ((char *)tmp_buf + recv_offset),
                                              total_count, recvtype, dst,
                                              MPIR_ALLGATHERV_TAG,
                                              comm, &status); 
                    /* for convenience, recv is posted for a bigger amount
                       than will be sent */ 
                    if (mpi_errno) return mpi_errno;
                    
                    NMPI_Get_count(&status, recvtype, &last_recv_cnt);
                    curr_cnt += last_recv_cnt;
                }
                
                /* if some processes in this process's subtree in this step
                   did not have any destination process to communicate with
                   because of non-power-of-two, we need to send them the
                   data that they would normally have received from those
                   processes. That is, the haves in this subtree must send to
                   the havenots. We use a logarithmic
                   recursive-halfing algorithm for this. */
                
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

                            offset = 0;
                            for (j=0; j<(my_tree_root+mask); j++)
                                offset += recvcounts[j];
                            offset *= recv_extent;

                            mpi_errno = MPIC_Send(((char *)tmp_buf + offset),
                                                  last_recv_cnt,
                                                  recvtype, dst,
                                                  MPIR_ALLGATHERV_TAG, comm); 
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

                            offset = 0;
                            for (j=0; j<(my_tree_root+mask); j++)
                                offset += recvcounts[j];
                            offset *= recv_extent;

                            mpi_errno = MPIC_Recv(((char *)tmp_buf + offset),
                                                  total_count, recvtype,
                                                  dst, MPIR_ALLGATHERV_TAG,
                                                  comm, &status);  
                            if (mpi_errno) return mpi_errno;
                            /* for convenience, recv is posted for a
                               bigger amount than will be sent */ 
                            
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

            /* copy data from tmp_buf to recvbuf */
            position = 0;
            for (j=0; j<comm_size; j++) {
                if ((sendbuf != MPI_IN_PLACE) || (j != rank)) {
                    /* not necessary to copy if in_place and
                       j==rank. otherwise copy. */
                    MPIR_Localcopy(((char *)tmp_buf + position*recv_extent),
                                   recvcounts[j], recvtype,
                                   ((char *)recvbuf + displs[j]*recv_extent),
                                   recvcounts[j], recvtype);
                }
                position += recvcounts[j];
            }

            MPIU_Free((char *)tmp_buf+true_lb); 
        }
        
#ifdef MPID_HAS_HETERO
        else {
            /* heterogeneous. need to use temp. buffer. */
            NMPI_Pack_size(total_count, recvtype, comm, &tmp_buf_size);
            tmp_buf = MPIU_Malloc(tmp_buf_size);
            if (!tmp_buf) { 
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                return mpi_errno;
            }
            
            /* calculate the value of nbytes, the number of bytes in packed
               representation corresponding to a single recvtype. Since
               MPI_Pack_size returns only an upper bound on 
               the size, to get the real size we actually pack some data
               into tmp_buf and see by how much 'position' is incremented. */
            
            position = 0;
            NMPI_Pack(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size,
                      &position, comm);
            nbytes = position;
            
            /* pack local data into right location in tmp_buf */
            position = 0;
            for (i=0; i<rank; i++) position += recvcounts[i];
            position *= nbytes;
            
            if (sendbuf != MPI_IN_PLACE) {
                NMPI_Pack(sendbuf, sendcount, sendtype, tmp_buf,
                          tmp_buf_size, &position, comm);
            }
            else {
                /* if in_place specified, local data is found in recvbuf */ 
                NMPI_Pack(((char *)recvbuf + displs[rank]*recv_extent), 
                          recvcounts[rank], recvtype, tmp_buf,
                          tmp_buf_size, &position, comm);
            }
            
            curr_cnt = recvcounts[rank]*nbytes;
            
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
                
                send_offset = 0;
                for (j=0; j<my_tree_root; j++)
                    send_offset += recvcounts[j];
                send_offset *= nbytes;
                
                recv_offset = 0;
                for (j=0; j<dst_tree_root; j++)
                    recv_offset += recvcounts[j];
                recv_offset *= nbytes;
                
                if (dst < comm_size) {
                    mpi_errno = MPIC_Sendrecv(((char *)tmp_buf + send_offset),
                                              curr_cnt, MPI_BYTE, dst,
                                              MPIR_ALLGATHERV_TAG,  
                                              ((char *)tmp_buf + recv_offset),
                                              nbytes*total_count, MPI_BYTE, dst,
                                              MPIR_ALLGATHERV_TAG, comm, &status);
                    /* for convenience, recv is posted for a bigger amount
                       than will be sent */ 
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
                    
                    offset = 0;
                    for (j=0; j<(my_tree_root+mask); j++)
                        offset += recvcounts[j];
                    offset *= nbytes;
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
                                                  dst, MPIR_ALLGATHERV_TAG,
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
                                                  nbytes*total_count, MPI_BYTE,
                                                  dst,
                                                  MPIR_ALLGATHERV_TAG,
                                                  comm, &status); 
                            /* for convenience, recv is posted for a bigger amount
                               than will be sent */ 
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
            for (j=0; j<comm_size; j++) {
                if ((sendbuf != MPI_IN_PLACE) || (j != rank)) {
                    /* not necessary to unpack if in_place and
                       j==rank. otherwise unpack. */
                    NMPI_Unpack(tmp_buf, tmp_buf_size, &position, 
                                ((char *)recvbuf + displs[j]*recv_extent),
                                recvcounts[j], recvtype, comm);
                }
            }
            
            MPIU_Free(tmp_buf);
        }
#endif /* MPID_HAS_HETERO */
    }

  /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

  return (mpi_errno);
}
/* end:nested */

/* begin:nested */
PMPI_LOCAL int MPIR_Allgatherv_inter ( 
    void *sendbuf, 
    int sendcount,  
    MPI_Datatype sendtype, 
    void *recvbuf, 
    int *recvcounts, 
    int *displs,   
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr )
{
/* Intercommunicator Allgatherv.
   This is done differently from the intercommunicator allgather
   because we don't have all the information to do a local
   intracommunictor gather (sendcount can be different on each
   process). Therefore, we do the following:
   Each group first does an intercommunicator gather to rank 0
   and then does an intracommunicator broadcast. 
*/

    int remote_size, mpi_errno, root, rank;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Datatype newtype;

    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* first do an intercommunicator gatherv from left to right group,
       then from right to left group */
    if (comm_ptr->is_low_group) {
        /* gatherv from right group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root,
                                 comm_ptr);  
        if (mpi_errno) return mpi_errno;
        /* gatherv to right group */
        root = 0;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root,
                                 comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }
    else {
        /* gatherv to left group  */
        root = 0;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root,
                                 comm_ptr);  
        if (mpi_errno) return mpi_errno;
        /* gatherv from left group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, root,
                                 comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }

    /* now do an intracommunicator broadcast within each group. we use
       a derived datatype to handle the displacements */

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
	MPIR_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    NMPI_Type_indexed(remote_size, recvcounts, displs, recvtype,
                      &newtype);
    NMPI_Type_commit(&newtype);

    mpi_errno = MPIR_Bcast(recvbuf, 1, newtype, 0, newcomm_ptr);

    NMPI_Type_free(&newtype);

    return mpi_errno;
}
/* end:nested */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allgatherv

/*@

MPI_Allgatherv - Gathers data from all tasks and deliver it to all

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcount - number of elements in send buffer (integer) 
. sendtype - data type of send buffer elements (handle) 
. recvcounts - integer array (of length group size) 
containing the number of elements that are received from each process 
. displs - integer array (of length group size). Entry 
 'i'  specifies the displacement (relative to recvbuf ) at
which to place the incoming data from process  'i'  
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

 This text was suggested by Rajeev Thakur, and has been adopted as a 
 clarification to the MPI standard.

.N Fortran

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
@*/
int MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Allgatherv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLGATHERV);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLGATHERV);

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
            int i, comm_size;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHERV);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }

	    if (comm_ptr->comm_kind == MPID_INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendcount, sendtype, mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                    MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                }
                MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
            }

            if (comm_ptr->comm_kind == MPID_INTRACOMM) 
                comm_size = comm_ptr->local_size;
            else
                comm_size = comm_ptr->remote_size;

            for (i=0; i<comm_size; i++) {
                MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                MPIR_ERRTEST_DATATYPE(recvcounts[i], recvtype, mpi_errno);
            }

            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
            }
            for (i=0; i<comm_size; i++) {
                if (recvcounts[i] > 0) {
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf,recvcounts[i],mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[i],recvtype,mpi_errno); 
                    break;
                }
            }
    
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHERV);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Allgatherv != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Allgatherv(sendbuf, sendcount,
                                                   sendtype, recvbuf,
                                                   recvcounts, displs,
                                                   recvtype, comm_ptr);
    }
    else
    {
	MPIR_Nest_incr();
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Allgatherv(sendbuf, sendcount, 
                                        sendtype, recvbuf,
                                        recvcounts, displs,
                                        recvtype, comm_ptr); 
        else {
            /* intracommunicator */
/*	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );*/
            mpi_errno = MPIR_Allgatherv_inter(sendbuf, sendcount, 
					      sendtype, recvbuf,
					      recvcounts, displs,
					      recvtype, comm_ptr); 
        }
	MPIR_Nest_decr();
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHERV);
	return MPI_SUCCESS;
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_allgatherv", "**mpi_allgatherv %p %d %D %p %p %p %D %C",
	    sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLGATHERV);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
