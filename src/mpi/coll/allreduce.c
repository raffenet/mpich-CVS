/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Allreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allreduce = PMPI_Allreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allreduce  MPI_Allreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allreduce as PMPI_Allreduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Allreduce PMPI_Allreduce

MPI_User_function *MPIR_Op_table[] = { MPIR_MAXF, MPIR_MINF, MPIR_SUM,
                                       MPIR_PROD, MPIR_LAND,
                                       MPIR_BAND, MPIR_LOR, MPIR_BOR,
                                       MPIR_LXOR, MPIR_BXOR,
                                       MPIR_MINLOC, MPIR_MAXLOC, };

MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[] = {
    MPIR_MAXF_check_dtype, MPIR_MINF_check_dtype,
    MPIR_SUM_check_dtype,
    MPIR_PROD_check_dtype, MPIR_LAND_check_dtype,
    MPIR_BAND_check_dtype, MPIR_LOR_check_dtype, MPIR_BOR_check_dtype,
    MPIR_LXOR_check_dtype, MPIR_BXOR_check_dtype,
    MPIR_MINLOC_check_dtype, MPIR_MAXLOC_check_dtype, }; 


/* This is the default implementation of allreduce. The algorithm is:
   
   Algorithm: MPI_Allreduce

   For the heterogeneous case, we call MPI_Reduce followed by MPI_Bcast
   in order to meet the requirement that all processes must have the
   same result. For the homogeneous case, we use the following algorithms.


   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see 
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html ).
   This algorithm implements the allreduce in two steps: first a
   reduce-scatter, followed by an allgather. A recursive-halving
   algorithm (beginning with processes that are distance 1 apart) is
   used for the reduce-scatter, and a recursive doubling 
   algorithm is used for the allgather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few even-numbered processes send their data to their right neighbors
   (rank+1), and the reduce-scatter and allgather happen among the remaining
   power-of-two processes. At the end, the first few even-numbered
   processes get the result from their right neighbors.

   For the power-of-two case, the cost for the reduce-scatter is 
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   allgather lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, 
   Cost = (2.floor(lgp)+2).alpha + (2.((p-1)/p) + 2).n.beta + n.(1+(p-1)/p).gamma

   
   For short messages, for user-defined ops, and for count < pof2 
   we use a recursive doubling algorithm (similar to the one in
   MPI_Allgather). We use this algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky. 

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

   Possible improvements: 

   End Algorithm: MPI_Allreduce
*/


PMPI_LOCAL int MPIR_Allreduce ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    static const char FCNAME[] = "MPIR_Allreduce";
    int is_homogeneous;
#ifdef MPID_HAS_HETERO
    int rc;
#endif
    int        comm_size, rank, type_size;
    int        mpi_errno = MPI_SUCCESS;
    int mask, dst, is_commutative, pof2, newrank, rem, newdst, i,
        send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps; 
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPICH_PerThread_t *p;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    
    if (count == 0) return MPI_SUCCESS;
    comm = comm_ptr->handle;
    
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    
#ifdef MPID_HAS_HETERO
    if (!is_homogeneous) {
        /* heterogeneous. To get the same result on all processes, we
           do a reduce to 0 and then broadcast. */
	MPIR_Nest_incr();
        mpi_errno = NMPI_Reduce ( sendbuf, recvbuf, count, datatype,
                                  op, 0, comm );
	/* FIXME: mpi_errno is error CODE, not necessarily the error
	   class MPI_ERR_OP.  In MPICH2, we can get the error class 
	   with 
	       errorclass = mpi_errno & ERROR_CLASS_MASK;
	*/
        if (mpi_errno == MPI_ERR_OP || mpi_errno == MPI_SUCCESS) {
	    /* Allow MPI_ERR_OP since we can continue from this error */
            rc = NMPI_Bcast  ( recvbuf, count, datatype, 0, comm );
            if (rc) mpi_errno = rc;
        }
	MPIR_Nest_decr();
    }
    else 
#endif /* MPID_HAS_HETERO */
	{
        /* homogeneous */
        
        /* set op_errno to 0. stored in perthread structure */
        MPID_GetPerThread(p);
        p->op_errno = 0;

        comm_size = comm_ptr->local_size;
        rank = comm_ptr->rank;
        
        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
            is_commutative = 1;
            /* get the function by indexing into the op table */
            uop = MPIR_Op_table[op%16 - 1];
        }
        else {
            MPID_Op_get_ptr(op, op_ptr);
            if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
                is_commutative = 0;
            else
                is_commutative = 1;
#ifdef HAVE_CXX_BINDING            
            if (op_ptr->language == MPID_LANG_CXX) {
                uop = (MPI_User_function *) op_ptr->function.c_function;
		is_cxx_uop = 1;
	    }
	    else
#endif
            if ((op_ptr->language == MPID_LANG_C))
                uop = (MPI_User_function *) op_ptr->function.c_function;
            else
                uop = (MPI_User_function *) op_ptr->function.f77_function;
        }
        
        /* need to allocate temporary buffer to store incoming data*/
        mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                              &true_extent); 
	if (mpi_errno) return mpi_errno;
        MPID_Datatype_get_extent_macro(datatype, extent);

        tmp_buf = MPIU_Malloc(count*(MPIR_MAX(extent,true_extent)));
        if (!tmp_buf) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
        
        /* copy local data into recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                       count, datatype);
            if (mpi_errno) return mpi_errno;
        }

        MPID_Datatype_get_size_macro(datatype, type_size);

        /* find nearest power-of-two less than or equal to comm_size */
        pof2 = 1;
        while (pof2 <= comm_size) pof2 <<= 1;
        pof2 >>=1;

        rem = comm_size - pof2;

        /* In the non-power-of-two case, all even-numbered
           processes of rank < 2*rem send their data to
           (rank+1). These even-numbered processes no longer
           participate in the algorithm until the very end. The
           remaining processes form a nice power-of-two. */
        
        /* check if multiple threads are calling this collective function */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

        if (rank < 2*rem) {
            if (rank % 2 == 0) { /* even */
                mpi_errno = MPIC_Send(recvbuf, count, 
                                      datatype, rank+1,
                                      MPIR_ALLREDUCE_TAG, comm);
                if (mpi_errno) return mpi_errno;
                
                /* temporarily set the rank to -1 so that this
                   process does not pariticipate in recursive
                   doubling */
                newrank = -1; 
            }
            else { /* odd */
                mpi_errno = MPIC_Recv(tmp_buf, count, 
                                      datatype, rank-1,
                                      MPIR_ALLREDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return mpi_errno;
                
                /* do the reduction on received data. since the
                   ordering is right, it doesn't matter whether
                   the operation is commutative or not. */
#ifdef HAVE_CXX_BINDING
                if (is_cxx_uop) {
                    (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
                                                    count,
                                                    datatype,
                                                    uop ); 
                }
                else 
#endif
                    (*uop)(tmp_buf, recvbuf, &count, &datatype);
                
                /* change the rank */
                newrank = rank / 2;
            }
        }
        else  /* rank >= 2*rem */
            newrank = rank - rem;
        
        /* If op is user-defined or count is less than pof2, use
           recursive doubling algorithm. Otherwise do a reduce-scatter
           followed by allgather. (If op is user-defined,
           derived datatypes are allowed and the user could pass basic
           datatypes on one process and derived on another as long as
           the type maps are the same. Breaking up derived
           datatypes to do the reduce-scatter is tricky, therefore
           using recursive doubling in that case.) */

        if (newrank != -1) {
            if ((count*type_size <= MPIR_ALLREDUCE_SHORT_MSG) ||
                (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||  
                (count < pof2)) { /* use recursive doubling */
                mask = 0x1;
                while (mask < pof2) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

                    /* Send the most current data, which is in recvbuf. Recv
                       into tmp_buf */ 
                    mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype, 
                                              dst, MPIR_ALLREDUCE_TAG, tmp_buf,
                                              count, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm,
                                              MPI_STATUS_IGNORE); 
                    if (mpi_errno) return mpi_errno;
                    
                    /* tmp_buf contains data received in this step.
                       recvbuf contains data accumulated so far */
                    
                    if (is_commutative  || (dst < rank)) {
                        /* op is commutative OR the order is already right */
#ifdef HAVE_CXX_BINDING
                        if (is_cxx_uop) {
                            (*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
                                                            count,
                                                            datatype,
                                                            uop ); 
                        }
                        else 
#endif
                            (*uop)(tmp_buf, recvbuf, &count, &datatype);
                    }
                    else {
                        /* op is noncommutative and the order is not right */
#ifdef HAVE_CXX_BINDING
                        if (is_cxx_uop) {
                            (*MPIR_Process.cxx_call_op_fn)( recvbuf, tmp_buf, 
                                                            count,
                                                            datatype,
                                                            uop ); 
                        }
                        else 
#endif
                            (*uop)(recvbuf, tmp_buf, &count, &datatype);
                        
                        /* copy result back into recvbuf */
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                                   recvbuf, count, datatype);
                        if (mpi_errno) return mpi_errno;
                    }
                    mask <<= 1;
                }
            }

            else {

                /* do a reduce-scatter followed by allgather */

                /* for the reduce-scatter, calculate the count that
                   each process receives and the displacement within
                   the buffer */

                cnts = (int *) MPIU_Malloc(pof2*sizeof(int));
                if (!cnts) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    return mpi_errno;
                }
                disps = (int *) MPIU_Malloc(pof2*sizeof(int));
                if (!disps) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    return mpi_errno;
                }

                for (i=0; i<(pof2-1); i++) 
                    cnts[i] = count/pof2;
                cnts[pof2-1] = count - (count/pof2)*(pof2-1);

                disps[0] = 0;
                for (i=1; i<pof2; i++)
                    disps[i] = disps[i-1] + cnts[i-1];

                mask = 0x1;
                send_idx = recv_idx = 0;
                last_idx = pof2;
                while (mask < pof2) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

                    send_cnt = recv_cnt = 0;
                    if (newrank < newdst) {
                        send_idx = recv_idx + pof2/(mask*2);
                        for (i=send_idx; i<last_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<send_idx; i++)
                            recv_cnt += cnts[i];
                    }
                    else {
                        recv_idx = send_idx + pof2/(mask*2);
                        for (i=send_idx; i<recv_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<last_idx; i++)
                            recv_cnt += cnts[i];
                    }

/*                    printf("Rank %d, send_idx %d, recv_idx %d, send_cnt %d, recv_cnt %d, last_idx %d\n", newrank, send_idx, recv_idx,
                           send_cnt, recv_cnt, last_idx);
                           */
                    /* Send data from recvbuf. Recv into tmp_buf */ 
                    mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                              disps[send_idx]*extent,
                                              send_cnt, datatype,  
                                              dst, MPIR_ALLREDUCE_TAG, 
                                              (char *) tmp_buf +
                                              disps[recv_idx]*extent,
                                              recv_cnt, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm,
                                              MPI_STATUS_IGNORE); 
                    if (mpi_errno) return mpi_errno;
                    
                    /* tmp_buf contains data received in this step.
                       recvbuf contains data accumulated so far */
                    
                    /* This algorithm is used only for predefined ops
                       and predefined ops are always commutative. */
#ifdef HAVE_CXX_BINDING
                    if (is_cxx_uop) {
                        (*MPIR_Process.cxx_call_op_fn)((char *) tmp_buf +
                                                       disps[recv_idx]*extent,
                                                       (char *) recvbuf + 
                                                       disps[recv_idx]*extent, 
                                                     recv_cnt, datatype, uop);
                    }
                    else 
#endif
                        (*uop)((char *) tmp_buf + disps[recv_idx]*extent,
                               (char *) recvbuf + disps[recv_idx]*extent, 
                               &recv_cnt, &datatype);
                    
                    /* update send_idx for next iteration */
                    send_idx = recv_idx;
                    mask <<= 1;

                    /* update last_idx, but not in last iteration
                       because the value is needed in the allgather
                       step below. */
                    if (mask < pof2)
                        last_idx = recv_idx + pof2/mask;
                }

                /* now do the allgather */

                mask >>= 1;
                while (mask > 0) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

                    send_cnt = recv_cnt = 0;
                    if (newrank < newdst) {
                        /* update last_idx except on first iteration */
                        if (mask != pof2/2)
                            last_idx = last_idx + pof2/(mask*2);

                        recv_idx = send_idx + pof2/(mask*2);
                        for (i=send_idx; i<recv_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<last_idx; i++)
                            recv_cnt += cnts[i];
                    }
                    else {
                        recv_idx = send_idx - pof2/(mask*2);
                        for (i=send_idx; i<last_idx; i++)
                            send_cnt += cnts[i];
                        for (i=recv_idx; i<send_idx; i++)
                            recv_cnt += cnts[i];
                    }

                    mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                              disps[send_idx]*extent,
                                              send_cnt, datatype,  
                                              dst, MPIR_ALLREDUCE_TAG, 
                                              (char *) recvbuf +
                                              disps[recv_idx]*extent,
                                              recv_cnt, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm,
                                              MPI_STATUS_IGNORE); 
                    if (mpi_errno) return mpi_errno;

                    if (newrank > newdst) send_idx = recv_idx;

                    mask >>= 1;
                }
                
                MPIU_Free(cnts);
                MPIU_Free(disps);
            }
        }

        /* In the non-power-of-two case, all odd-numbered
           processes of rank < 2*rem send the result to
           (rank-1), the ranks who didn't participate above. */
        if (rank < 2*rem) {
            if (rank % 2)  /* odd */
                mpi_errno = MPIC_Send(recvbuf, count, 
                                      datatype, rank-1,
                                      MPIR_ALLREDUCE_TAG, comm);
            else  /* even */
                mpi_errno = MPIC_Recv(recvbuf, count,
                                      datatype, rank+1,
                                      MPIR_ALLREDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE); 
            
            if (mpi_errno) return mpi_errno;
        }

        /* check if multiple threads are calling this collective function */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

        MPIU_Free((char *)tmp_buf+true_lb); 
            
        if (p->op_errno) mpi_errno = p->op_errno;
    }
    
    return (mpi_errno);
}


#ifdef USE_OLD
PMPI_LOCAL int MPIR_Allreduce ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    int rc, is_homogeneous;
    int        comm_size, rank;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int mask, dst, dst_tree_root, my_tree_root, nprocs_completed, k, i,
        j, tmp_mask, tree_root, is_commutative; 
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPICH_PerThread_t *p;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    
    if (count == 0) return MPI_SUCCESS;
    comm = comm_ptr->handle;
    
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    
    if (!is_homogeneous) {
        /* heterogeneous. To get the same result on all processes, we
           do a reduce to 0 and then broadcast. */
	MPIR_Nest_incr();
        mpi_errno = NMPI_Reduce ( sendbuf, recvbuf, count, datatype,
                                  op, 0, comm );
        if (mpi_errno == MPI_ERR_OP || mpi_errno == MPI_SUCCESS) {
	    /* Allow MPI_ERR_OP since we can continue from this error */
            rc = NMPI_Bcast  ( recvbuf, count, datatype, 0, comm );
            if (rc) mpi_errno = rc;
        }
	MPIR_Nest_decr();
    }
    else {
        /* homogeneous. Use recursive doubling algorithm similar to the
           one used in all_gather */
        
        /* set op_errno to 0. stored in perthread structure */
        MPID_GetPerThread(p);
        p->op_errno = 0;

        comm_size = comm_ptr->local_size;
        rank = comm_ptr->rank;
        
        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
            is_commutative = 1;
            /* get the function by indexing into the op table */
            uop = MPIR_Op_table[op%16 - 1];
        }
        else {
            MPID_Op_get_ptr(op, op_ptr);
            if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
                is_commutative = 0;
            else
                is_commutative = 1;
#ifdef HAVE_CXX_BINDING            
            if (op_ptr->language == MPID_LANG_CXX) {
                uop = (MPI_User_function *) op_ptr->function.c_function;
		is_cxx_uop = 1;
	    }
	    else
#endif
            if ((op_ptr->language == MPID_LANG_C))
                uop = (MPI_User_function *) op_ptr->function.c_function;
            else
                uop = (MPI_User_function *) op_ptr->function.f77_function;
        }
        
        /* need to allocate temporary buffer to store incoming data*/
        mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                              &true_extent); 
	if (mpi_errno) return mpi_errno;
        MPID_Datatype_get_extent_macro( datatype, extent );

        tmp_buf = MPIU_Malloc(count*(MPIR_MAX(extent,true_extent)));
        if (!tmp_buf) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
        
        /* copy local data into recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                       count, datatype);
            if (mpi_errno) return mpi_errno;
        }
        
        /* check if multiple threads are calling this collective function */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
        
        mask = 0x1;
        i = 0;
        while (mask < comm_size) {
            dst = rank ^ mask;
            
            dst_tree_root = dst >> i;
            dst_tree_root <<= i;
            
            my_tree_root = rank >> i;
            my_tree_root <<= i;
            
            if (dst < comm_size) {
                /* Send most current data, which is in recvbuf. Recv
                   into tmp_buf */ 
                mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype,
                                          dst, MPIR_ALLREDUCE_TAG, tmp_buf,
                                          count, datatype, dst,
                                          MPIR_ALLREDUCE_TAG, comm,
                                          &status); 
                if (mpi_errno) return mpi_errno;
                
                /* tmp_buf contains data received in this step.
                   recvbuf contains data accumulated so far */
                
                if (is_commutative  || (dst_tree_root < my_tree_root)) {
                    /* op is commutative OR the order is already right */
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			(*MPIR_Process.cxx_call_op_fn)( tmp_buf, recvbuf, 
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(tmp_buf, recvbuf, &count, &datatype);
                }
                else {
                    /* op is noncommutative and the order is not right */
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			(*MPIR_Process.cxx_call_op_fn)( recvbuf, tmp_buf, 
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(recvbuf, tmp_buf, &count, &datatype);

                    /* copy result back into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               recvbuf, count, datatype);
                    if (mpi_errno) return mpi_errno;
                }
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
                        /* send the current result */
                        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                                              dst, MPIR_ALLREDUCE_TAG,
                                              comm);  
                        if (mpi_errno) return mpi_errno;
                    }
                    /* recv only if this proc. doesn't have data and sender
                       has data */
                    else if ((dst < rank) && 
                             (dst < tree_root + nprocs_completed) &&
                             (rank >= tree_root + nprocs_completed)) {
                        mpi_errno = MPIC_Recv(recvbuf, count, datatype,
                                              dst, MPIR_ALLREDUCE_TAG, comm,
                                              &status); 
                        if (mpi_errno) return mpi_errno;
                    }
                    tmp_mask >>= 1;
                    k--;
                }
            }
            mask <<= 1;
            i++;
        }
        
        MPIU_Free((char *)tmp_buf+true_lb); 
        
        /* check if multiple threads are calling this collective function */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

        if (p->op_errno) mpi_errno = p->op_errno;
    }
    
    return (mpi_errno);
}
#endif

PMPI_LOCAL int MPIR_Allreduce_inter ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
/* Intercommunicator Allreduce.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator broadcasts in each group.

   We don't do local reduces first and then intercommunicator
   broadcasts because it would require allocation of a temporary buffer. 
*/

    int rank, mpi_errno, root;
    MPID_Comm *newcomm_ptr = NULL;

    rank = comm_ptr->rank;

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
	MPIR_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Bcast(recvbuf, count, datatype, 0, newcomm_ptr);
    if (mpi_errno) return mpi_errno;

    return mpi_errno;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allreduce

/*@
MPI_Allreduce - Combines values from all processes and distributes the result
                back to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - starting address of receive buffer (choice) 

.N Fortran

.N collops

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_OP
.N MPI_ERR_COMM
@*/
int MPI_Allreduce ( void *sendbuf, void *recvbuf, int count, 
		    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm )
{
    static const char FCNAME[] = "MPI_Allreduce";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLREDUCE);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_ALLREDUCE);

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
            MPID_Datatype *datatype_ptr = NULL;
            MPID_Op *op_ptr = NULL;

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            }

	    if (mpi_errno != MPI_SUCCESS) {
		MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = 
                    ( * MPIR_Op_check_dtype_table[op%16 - 1] )(datatype); 
            }
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Allreduce != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Allreduce(sendbuf, recvbuf, count,
                                              datatype, op, comm_ptr);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype,
                                       op, comm_ptr); 
        else {
            /* intercommunicator */
/*	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );*/
            mpi_errno = MPIR_Allreduce_inter(sendbuf, recvbuf, count,
					     datatype, op, comm_ptr);       
        }
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
