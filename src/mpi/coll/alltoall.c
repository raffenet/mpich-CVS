/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Alltoall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alltoall = PMPI_Alltoall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alltoall  MPI_Alltoall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alltoall as PMPI_Alltoall
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Alltoall PMPI_Alltoall

/* This is the default implementation of alltoall. The algorithm is:
   
   Algorithm: MPI_Alltoall

   We use four algorithms for alltoall. For very short messages, we use
   a recursive doubling algorithm that takes lgp steps. At each step
   pairs of processes exchange all the data they have (received) so
   far. A lot more data is communicated than each process needs, but
   for very short messages (typically 256 bytes or less), this
   algorithm is still better because of the lower latency.

   Cost = lgp.alpha + n.p.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   For medium size messages (typically 256 bytes -- 256 Kbytes), we
   use an algorithm that posts all irecvs and isends and then does a
   waitall. We scatter the order of sources and destinations among the
   processes, so that all processes don't try to send/recv to/from the
   same process at the same time.

   For long messages, we use a pairwise exchange algorithm, which
   takes p-1 steps. For a power-of-two number of processes, we
   calculate the pairs by using an exclusive-or algorithm:
           for (i=1; i<comm_size; i++)
               dest = rank ^ i;
   This algorithm doesn't work if the number of processes is not a power of
   two. For a non-power-of-two number of processes, we create pairs by
   having each process receive from (rank-i) and send to (rank+i) at
   step i.

   Cost = (p-1).alpha + n.beta

   where n is the total amount of data a process needs to send to all
   other processes.

   Possible improvements: 

   End Algorithm: MPI_Alltoall
*/

/* begin:nested */
PMPI_LOCAL int MPIR_Alltoall( 
    void *sendbuf, 
    int sendcount, 
    MPI_Datatype sendtype, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr )
{
    static const char FCNAME[] = "MPIR_Alltoall";
    int          comm_size, i, j, k, p, pof2;
    MPI_Aint     sendtype_extent, recvtype_extent;
    MPI_Aint sendtype_true_extent, sendbuf_extent, sendtype_true_lb;
    int          mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank, nbytes, curr_cnt, dst_tree_root, my_tree_root;
    int last_recv_cnt, mask, tmp_mask, tree_root, nprocs_completed;
    int sendtype_size;
    void *tmp_buf;
    MPI_Comm comm;
    MPI_Request *reqarray;
    MPI_Status *starray;
    
    if (sendcount == 0) return MPI_SUCCESS;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPID_Datatype_get_extent_macro(sendtype, sendtype_extent);

    /* get true extent of sendtype */
    mpi_errno = NMPI_Type_get_true_extent(sendtype, &sendtype_true_lb,
                                          &sendtype_true_extent);  
    if (mpi_errno) return mpi_errno;

    MPID_Datatype_get_size_macro(sendtype, sendtype_size);
    nbytes = sendtype_size * sendcount * comm_size;
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
    
    if (nbytes <= MPIR_ALLTOALL_SHORT_MSG) {
        /* Short message. Use recursive doubling. Each process sends all
           its data at each step along with all data it received in
           previous steps. */
        
        /* need to allocate temporary buffer of size
           sendbuf_extent*comm_size */
        
        sendbuf_extent = sendcount * comm_size *
            (MPIR_MAX(sendtype_true_extent, sendtype_extent));
        tmp_buf = MPIU_Malloc(sendbuf_extent*comm_size);
        if (!tmp_buf) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - sendtype_true_lb);
        
        /* copy local sendbuf into tmp_buf at location indexed by rank */
        curr_cnt = sendcount*comm_size;
        mpi_errno = MPIR_Localcopy(sendbuf, curr_cnt, sendtype,
                                   ((char *)tmp_buf + rank*sendbuf_extent),
                                   curr_cnt, sendtype);
        if (mpi_errno) return mpi_errno;
        
        mask = 0x1;
        i = 0;
        while (mask < comm_size) {
            dst = rank ^ mask;
            
            dst_tree_root = dst >> i;
            dst_tree_root <<= i;
            
            my_tree_root = rank >> i;
            my_tree_root <<= i;
            
            if (dst < comm_size) {
                mpi_errno = MPIC_Sendrecv(((char *)tmp_buf +
                                           my_tree_root*sendbuf_extent),
                                          curr_cnt, sendtype,
                                          dst, MPIR_ALLTOALL_TAG, 
                                          ((char *)tmp_buf +
                                           dst_tree_root*sendbuf_extent),
                                          sendcount*comm_size*mask,
                                          sendtype, dst, MPIR_ALLTOALL_TAG, 
                                          comm, &status);
                if (mpi_errno) return mpi_errno;
                
                /* in case of non-power-of-two nodes, less data may be
                   received than specified */
                NMPI_Get_count(&status, sendtype, &last_recv_cnt);
                curr_cnt += last_recv_cnt;
            }
            
            /* if some processes in this process's subtree in this step
               did not have any destination process to communicate with
               because of non-power-of-two, we need to send them the
               result. We use a logarithmic recursive-halfing algorithm
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
                        /* send the data received in this step above */
                        mpi_errno = MPIC_Send(((char *)tmp_buf +
                                               dst_tree_root*sendbuf_extent),
                                              last_recv_cnt, sendtype,
                                              dst, MPIR_ALLTOALL_TAG,
                                              comm);  
                        if (mpi_errno) return mpi_errno;
                    }
                    /* recv only if this proc. doesn't have data and sender
                       has data */
                    else if ((dst < rank) && 
                             (dst < tree_root + nprocs_completed) &&
                             (rank >= tree_root + nprocs_completed)) {
                        mpi_errno = MPIC_Recv(((char *)tmp_buf +
                                               dst_tree_root*sendbuf_extent),
                                              sendcount*comm_size*mask, 
                                              sendtype,   
                                              dst, MPIR_ALLTOALL_TAG,
                                              comm, &status); 
                        if (mpi_errno) return mpi_errno;
                        NMPI_Get_count(&status, sendtype, &last_recv_cnt);
                        curr_cnt += last_recv_cnt;
                    }
                    tmp_mask >>= 1;
                    k--;
                }
            }
            
            mask <<= 1;
            i++;
        }
        
        /* now copy everyone's contribution from tmp_buf to recvbuf */
        for (p=0; p<comm_size; p++) {
            mpi_errno = MPIR_Localcopy(((char *)tmp_buf +
                                        p*sendbuf_extent +
                                        rank*sendcount*sendtype_extent),
                                        sendcount, sendtype, 
                                        ((char*)recvbuf +
                                         p*recvcount*recvtype_extent), 
                                        recvcount, recvtype);
            if (mpi_errno) return mpi_errno;
        }
        
        MPIU_Free((char *)tmp_buf+sendtype_true_lb); 
    }

    else if ((nbytes > MPIR_ALLTOALL_SHORT_MSG) && 
             (nbytes <= MPIR_ALLTOALL_MEDIUM_MSG)) {  
        /* Medium-size message. Use isend/irecv with scattered
           destinations */

        reqarray = (MPI_Request *) MPIU_Malloc(2*comm_size*sizeof(MPI_Request));
        starray = (MPI_Status *) MPIU_Malloc(2*comm_size*sizeof(MPI_Status));

        /* do the communication -- post all sends and receives: */
        for ( i=0; i<comm_size; i++ ) { 
            dst = (rank+i) % comm_size;
            mpi_errno = MPIC_Irecv((char *)recvbuf +
                                  dst*recvcount*recvtype_extent, 
                                  recvcount, recvtype, dst,
                                  MPIR_ALLTOALL_TAG, comm,
                                  &reqarray[i]);
            if (mpi_errno) return mpi_errno;
        }

        for ( i=0; i<comm_size; i++ ) { 
            dst = (rank+i) % comm_size;
            mpi_errno = MPIC_Isend((char *)sendbuf +
                                   dst*sendcount*sendtype_extent, 
                                   sendcount, sendtype, dst,
                                   MPIR_ALLTOALL_TAG, comm,
                                   &reqarray[i+comm_size]);
            if (mpi_errno) return mpi_errno;
        }
  
        /* ... then wait for *all* of them to finish: */
        mpi_errno = NMPI_Waitall(2*comm_size,reqarray,starray);
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (j=0; j<2*comm_size; j++) {
                if (starray[j].MPI_ERROR != MPI_SUCCESS) 
                    mpi_errno = starray[j].MPI_ERROR;
            }
        }

        MPIU_Free(starray);
        MPIU_Free(reqarray);
    }

    else {
        /* Long message. Use pairwise exchange. If comm_size is a
           power-of-two, use exclusive-or to create pairs. Else send
           to rank+i, receive from rank-i. */
        
        /* Make local copy first */
        mpi_errno = MPIR_Localcopy(((char *)sendbuf + 
                                    rank*sendcount*sendtype_extent), 
                                   sendcount, sendtype, 
                                   ((char *)recvbuf +
                                    rank*recvcount*recvtype_extent),
                                   recvcount, recvtype);
        if (mpi_errno) return mpi_errno;

        /* Is comm_size a power-of-two? */
        i = 1;
        while (i < comm_size)
            i *= 2;
        if (i == comm_size)
            pof2 = 1;
        else 
            pof2 = 0;

        /* Do the pairwise exchanges */
        for (i=1; i<comm_size; i++) {
            if (pof2 == 1) {
                /* use exclusive-or algorithm */
                src = dst = rank ^ i;
            }
            else {
                src = (rank - i + comm_size) % comm_size;
                dst = (rank + i) % comm_size;
            }

            mpi_errno = MPIC_Sendrecv(((char *)sendbuf +
                                       dst*sendcount*sendtype_extent), 
                                      sendcount, sendtype, dst,
                                      MPIR_ALLTOALL_TAG, 
                                      ((char *)recvbuf +
                                       src*recvcount*recvtype_extent),
                                      recvcount, recvtype, src,
                                      MPIR_ALLTOALL_TAG, comm, &status);
            if (mpi_errno) return mpi_errno;
        }
    }
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    
    return (mpi_errno);
}
/* end:nested */

/* begin:nested */
PMPI_LOCAL int MPIR_Alltoall_inter( 
    void *sendbuf, 
    int sendcount, 
    MPI_Datatype sendtype, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype recvtype, 
    MPID_Comm *comm_ptr )
{
/* Intercommunicator alltoall. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoall for long
   messages. Since the local and remote groups can be of different
   sizes, we first compute the max of local_group_size,
   remote_group_size. At step i, 0 <= i < max_size, each process
   receives from src = (rank - i + max_size) % max_size if src <
   remote_size, and sends to dst = (rank + i) % max_size if dst <
   remote_size. 
*/

    int          local_size, remote_size, max_size, i;
    MPI_Aint     sendtype_extent, recvtype_extent;
    int          mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int src, dst, rank;
    char *sendaddr, *recvaddr;
    MPI_Comm comm;
    
    local_size = comm_ptr->local_size; 
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;
    comm = comm_ptr->handle;

    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPID_Datatype_get_extent_macro(recvtype, recvtype_extent);
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
    
    /* Do the pairwise exchanges */
    max_size = MPIR_MAX(local_size, remote_size);
    for (i=0; i<max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
        }
        else {
            recvaddr = (char *)recvbuf + src*recvcount*recvtype_extent;
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
        }
        else {
            sendaddr = (char *)sendbuf + dst*sendcount*sendtype_extent;
        }

        mpi_errno = MPIC_Sendrecv(sendaddr, sendcount, sendtype, dst, 
                                  MPIR_ALLTOALL_TAG, recvaddr,
                                  recvcount, recvtype, src,
                                  MPIR_ALLTOALL_TAG, comm, &status);
        if (mpi_errno) return mpi_errno;
    }

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    
    return (mpi_errno);
}
/* end:nested */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alltoall

/*@
MPI_Alltoall - Sends data from all to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcount - number of elements to send to each process (integer) 
. sendtype - data type of send buffer elements (handle) 
. recvcount - number of elements received from any process (integer) 
. recvtype - data type of receive buffer elements (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - address of receive buffer (choice) 

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Alltoall";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLTOALL);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLTOALL);

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
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALL);
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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALL);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Alltoall != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Alltoall(sendbuf, sendcount,
                                                 sendtype, recvbuf, recvcount,
                                                 recvtype, comm_ptr);
    }
    else
    {
	MPIR_Nest_incr();
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Alltoall(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcount, recvtype, comm_ptr); 
        else {
            /* intercommunicator */
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );
            /*mpi_errno = MPIR_Alltoall_inter(sendbuf, sendcount,
                                            sendtype, recvbuf,
                                            recvcount, recvtype,
                                            comm_ptr); */
        }
	MPIR_Nest_decr();
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALL);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALL);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */

    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLTOALL);
    return MPI_SUCCESS;
}
