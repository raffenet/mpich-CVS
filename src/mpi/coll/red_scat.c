/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Reduce_scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce_scatter = PMPI_Reduce_scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce_scatter  MPI_Reduce_scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce_scatter as PMPI_Reduce_scatter
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Reduce_scatter PMPI_Reduce_scatter
/* This is the default implementation of reduce_scatter. The algorithm is:
   
   Algorithm: MPI_Reduce_scatter

   For long messages, we use a pairwise exchange algorithm similar to
   the one used in MPI_Alltoall. At step i, each process sends n/p
   amount of data to (rank+i) and receives n/p amount of data from 
   (rank-i).
   Cost = (p-1).alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma

   For short messages, we use a recursive doubling algorithm, which
   takes lgp steps. At step 1, processes exchange (n-n/p) amount of
   data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
   amount of data, and so forth.

   Cost = lgp.alpha + n.(lgp-(p-1)/p).beta + n.(lgp-(p-1)/p).gamma

   Possible improvements: 

   End Algorithm: MPI_Reduce_scatter
*/


PMPI_LOCAL int MPIR_Reduce_scatter ( 
    void *sendbuf, 
    void *recvbuf, 
    int *recvcnts, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    int   rank, comm_size, i;
    MPI_Aint extent, lb=0; 
    int  *displs;
    void *tmp_recvbuf, *tmp_results;
    int   mpi_errno = MPI_SUCCESS;
    int type_size, dis[2], blklens[2], total_count, nbytes, src, dst;
    int mask, dst_tree_root, my_tree_root, j, k;
    MPI_Datatype sendtype, recvtype;
    int nprocs_completed, tmp_mask, tree_root, is_commutative;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Status status;
    MPI_Comm comm;
    MPICH_PerThread_t *p;
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    MPID_GetPerThread(p);
    p->op_errno = 0;

    MPID_Datatype_get_size_macro(datatype, type_size);
    MPID_Datatype_get_extent_macro(datatype, extent);
#ifdef UNIMPLEMENTED
    MPI_Type_lb( datatype, &lb );
#endif
    
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
        if ((op_ptr->language == MPID_LANG_C) || (op_ptr->language ==
                                                  MPID_LANG_CXX)) 
#else
        if ((op_ptr->language == MPID_LANG_C))
#endif
            uop = (MPI_User_function *) op_ptr->function.c_function;
        else
            uop = (MPI_User_function *) op_ptr->function.f77_function;
    }

    displs = MPIU_Malloc(comm_size*sizeof(int));
    if (!displs) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        displs[i] = total_count;
        total_count += recvcnts[i];
    }
    
    nbytes = total_count * type_size;
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );

    if (nbytes >= MPIR_REDUCE_SCATTER_SHORT_MSG) {
        /* for long messages, use (p-1) pairwise exchanges */ 
        
        if (sendbuf != MPI_IN_PLACE) {
            /* copy local data into recvbuf */
            mpi_errno = MPIR_Localcopy(((char *)sendbuf+displs[rank]*extent),
                                       recvcnts[rank], datatype, recvbuf,
                                       recvcnts[rank], datatype);
            if (mpi_errno) return mpi_errno;
        }
        
        /* allocate temporary buffer to store incoming data */
        tmp_recvbuf = MPIU_Malloc(extent*recvcnts[rank]);
        if (!tmp_recvbuf) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        /* adjust for potential negative lower bound in datatype */
        /* MPI_Type_lb HAS NOT BEEN IMPLEMENTED YET. BUT lb IS
           INITIALIZED TO 0, AND DERIVED DATATYPES AREN'T SUPPORTED YET,
           SO IT'S OK */
        tmp_recvbuf = (void *)((char*)tmp_recvbuf - lb);
        
        for (i=1; i<comm_size; i++) {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
            
            /* send the data that dst needs. recv data that this process
               needs from src into tmp_recvbuf */
            if (sendbuf != MPI_IN_PLACE) 
                mpi_errno = MPIC_Sendrecv(((char *)sendbuf+displs[dst]*extent), 
                                          recvcnts[dst], datatype, dst,
                                          MPIR_REDUCE_SCATTER_TAG, tmp_recvbuf,
                                          recvcnts[rank], datatype, src,
                                          MPIR_REDUCE_SCATTER_TAG, comm,
                                          &status);
            else
                mpi_errno = MPIC_Sendrecv(((char *)recvbuf+displs[dst]*extent), 
                                          recvcnts[dst], datatype, dst,
                                          MPIR_REDUCE_SCATTER_TAG, tmp_recvbuf,
                                          recvcnts[rank], datatype, src,
                                          MPIR_REDUCE_SCATTER_TAG, comm,
                                          &status);

            if (mpi_errno) return mpi_errno;
            
            if (is_commutative || (src < rank)) {
                if (sendbuf != MPI_IN_PLACE)
                    (*uop)(tmp_recvbuf, recvbuf, &recvcnts[rank], &datatype); 
                else {
                    (*uop)(tmp_recvbuf, ((char *)recvbuf+displs[rank]*extent), 
                           &recvcnts[rank], &datatype); 
                    /* we can't store the result at the beginning of
                       recvbuf right here because there is useful data
                       there that other process/processes need. at the
                       end, we will copy back the result to the
                       beginning of recvbuf. */
                }
            }
            else {
                if (sendbuf != MPI_IN_PLACE) {
                    (*uop)(recvbuf, tmp_recvbuf, &recvcnts[rank], &datatype); 
                    /* copy result back into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_recvbuf, recvcnts[rank], 
                                               datatype, recvbuf,
                                               recvcnts[rank], datatype); 
                }
                else {
                    (*uop)(((char *)recvbuf+displs[rank]*extent),
                           tmp_recvbuf, &recvcnts[rank], &datatype);   
                    /* copy result back into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_recvbuf, recvcnts[rank], 
                                               datatype, 
                                               ((char *)recvbuf +
                                                displs[rank]*extent), 
                                               recvcnts[rank], datatype); 
                }
                if (mpi_errno) return mpi_errno;
            }
        }
        
        MPIU_Free((char *)tmp_recvbuf+lb); 

        /* if MPI_IN_PLACE, move output data to the beginning of
           recvbuf. already done for rank 0. */
        if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
            mpi_errno = MPIR_Localcopy(((char *)recvbuf +
                                        displs[rank]*extent),  
                                       recvcnts[rank], datatype, 
                                       recvbuf, 
                                       recvcnts[rank], datatype); 
            if (mpi_errno) return mpi_errno;
        }
    }
    
    else {
        /* for short messages, use recursive doubling. */

        printf("ERROR: MPI_Reduce_scatter not implemented for short messages because it needs derived datatypes\n");
        NMPI_Abort(MPI_COMM_WORLD, 1);     
        
        /* need to allocate temporary buffer to receive incoming data*/
        tmp_recvbuf = MPIU_Malloc(extent*total_count);
        if (!tmp_recvbuf) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        /* adjust for potential negative lower bound in datatype */
        /* MPI_Type_lb HAS NOT BEEN IMPLEMENTED YET. BUT lb IS
           INITIALIZED TO 0, AND DERIVED DATATYPES AREN'T SUPPORTED YET,
           SO IT'S OK */
        tmp_recvbuf = (void *)((char*)tmp_recvbuf - lb);
        
        /* need to allocate another temporary buffer to accumulate
           results */
        tmp_results = MPIU_Malloc(extent*total_count);
        if (!tmp_results) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }        
        /* adjust for potential negative lower bound in datatype */
        /* MPI_Type_lb HAS NOT BEEN IMPLEMENTED YET. BUT lb IS
           INITIALIZED TO 0, AND DERIVED DATATYPES AREN'T SUPPORTED YET,
           SO IT'S OK */
        tmp_results = (void *)((char*)tmp_results - lb);
        
        /* copy sendbuf into tmp_results */
        if (sendbuf != MPI_IN_PLACE)
            mpi_errno = MPIR_Localcopy(sendbuf, total_count, datatype,
                                       tmp_results, total_count, datatype);
        else
            mpi_errno = MPIR_Localcopy(recvbuf, total_count, datatype,
                                       tmp_results, total_count, datatype);
            
        if (mpi_errno) return mpi_errno;

        
        mask = 0x1;
        i = 0;
        while (mask < comm_size) {
            dst = rank ^ mask;
            
            dst_tree_root = dst >> i;
            dst_tree_root <<= i;
            
            my_tree_root = rank >> i;
            my_tree_root <<= i;
            
            /* At step 1, processes exchange (n-n/p) amount of
               data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
               amount of data, and so forth. We use derived datatypes for this.
               
               At each step, a process does not need to send data
               indexed from my_tree_root to
               my_tree_root+mask-1. Similarly, a process won't receive
               data indexed from dst_tree_root to dst_tree_root+mask-1. */
            
            /* calculate sendtype */
            blklens[0] = blklens[1] = 0;
            for (j=0; j<my_tree_root; j++)
                blklens[0] += recvcnts[j];
            for (j=my_tree_root+mask; j<comm_size; j++)
                blklens[1] += recvcnts[j];
            
            dis[0] = 0;
            dis[1] = blklens[0];
            for (j=my_tree_root; j<my_tree_root+mask; j++)
                dis[1] += recvcnts[j];
            
#ifdef UNIMPLEMENTED
            NMPI_Type_indexed(2, blklens, dis, datatype, &sendtype);
            NMPI_Type_commit(&sendtype);
#endif
            
            /* calculate recvtype */
            blklens[0] = blklens[1] = 0;
            for (j=0; j<dst_tree_root; j++)
                blklens[0] += recvcnts[j];
            for (j=dst_tree_root+mask; j<comm_size; j++)
                blklens[1] += recvcnts[j];
            
            dis[0] = 0;
            dis[1] = blklens[0];
            for (j=dst_tree_root; j<dst_tree_root+mask; j++)
                dis[1] += recvcnts[j];
            
#ifdef UNIMPLEMENTED
            NMPI_Type_indexed(2, blklens, dis, datatype, &recvtype);
            NMPI_Type_commit(&recvtype);
#endif
            
            if (dst < comm_size) {
                /* tmp_results contains data to be sent in each step. Data is
                   received in tmp_recvbuf and then accumulated into
                   tmp_results. */ 
                
                mpi_errno = MPIC_Sendrecv(tmp_results, 1, sendtype, dst,
                                          MPIR_REDUCE_SCATTER_TAG, 
                                          tmp_recvbuf, 1, recvtype, dst,
                                          MPIR_REDUCE_SCATTER_TAG, comm,
                                          &status); 
                if (mpi_errno) return mpi_errno;
                
                if (is_commutative || (dst_tree_root < my_tree_root)) {
                    (*uop)(tmp_recvbuf, tmp_results, &blklens[0],
                           &datatype); 
                    (*uop)(((char *)tmp_recvbuf + dis[1]*extent),
                           ((char *)tmp_results + dis[1]*extent),
                           &blklens[1], &datatype); 
                }
                else {
                    (*uop)(tmp_results, tmp_recvbuf, &blklens[0],
                           &datatype); 
                    (*uop)(((char *)tmp_results + dis[1]*extent),
                           ((char *)tmp_recvbuf + dis[1]*extent),
                           &blklens[1], &datatype); 
                    /* copy result back into tmp_results */
                    mpi_errno = MPIC_Sendrecv(tmp_recvbuf, 1, recvtype, rank,
                                              MPIR_REDUCE_SCATTER_TAG, 
                                              tmp_results, 1, recvtype, rank,
                                              MPIR_REDUCE_SCATTER_TAG, 
                                              comm, &status);
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
                        mpi_errno = MPIC_Send(tmp_recvbuf, 1, recvtype,
                                              dst, MPIR_REDUCE_SCATTER_TAG,
                                              comm);  
                        if (mpi_errno) return mpi_errno;
                    }
                    /* recv only if this proc. doesn't have data and sender
                       has data */
                    else if ((dst < rank) && 
                             (dst < tree_root + nprocs_completed) &&
                             (rank >= tree_root + nprocs_completed)) {
                        mpi_errno = MPIC_Recv(tmp_recvbuf, 1, recvtype, dst,
                                              MPIR_REDUCE_SCATTER_TAG,
                                              comm, &status); 
                        if (mpi_errno) return mpi_errno;
                        
                        if (is_commutative || (dst_tree_root <
                                               my_tree_root)) { 
                            (*uop)(tmp_recvbuf, tmp_results, &blklens[0],
                                   &datatype); 
                            (*uop)(((char *)tmp_recvbuf + dis[1]*extent),
                                   ((char *)tmp_results + dis[1]*extent),
                                   &blklens[1], &datatype); 
                        }
                        else {
                            (*uop)(tmp_results, tmp_recvbuf, &blklens[0],
                                   &datatype); 
                            (*uop)(((char *)tmp_results + dis[1]*extent),
                                   ((char *)tmp_recvbuf + dis[1]*extent),
                                   &blklens[1], &datatype); 
                            /* copy result back into tmp_results */
                            mpi_errno = MPIC_Sendrecv(tmp_recvbuf, 1,
                                                      recvtype, rank, 
                                                      MPIR_REDUCE_SCATTER_TAG, 
                                                      tmp_results, 1,
                                                      recvtype, rank, 
                                                      MPIR_REDUCE_SCATTER_TAG, 
                                                      comm, &status);
                            if (mpi_errno) return mpi_errno;
                        }
                    }
                    tmp_mask >>= 1;
                    k--;
                }
            }
            
#ifdef UNIMPLEMENTED
            NMPI_Type_free(&sendtype);
            NMPI_Type_free(&recvtype);
#endif
            
            mask <<= 1;
            i++;
        }

        /* now copy final results from tmp_results to recvbuf */
        mpi_errno = MPIR_Localcopy(((char *)tmp_results+displs[rank]*extent),
                                   recvcnts[rank], datatype, recvbuf,
                                   recvcnts[rank], datatype); 

        if (mpi_errno) return mpi_errno;
        
        MPIU_Free((char *)tmp_recvbuf+lb); 
        MPIU_Free((char *)tmp_results+lb); 
    }
    
    MPIU_Free(displs);
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );

    if (p->op_errno) mpi_errno = p->op_errno;

    return (mpi_errno);
}


PMPI_LOCAL int MPIR_Reduce_scatter_inter ( 
    void *sendbuf, 
    void *recvbuf, 
    int *recvcnts, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
/* Intercommunicator Reduce_scatter.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator scattervs in each group.
*/
    
    int rank, mpi_errno, inleftgroup, root, local_size, total_count, i;
    MPI_Comm newcomm;
    MPI_Group group;
    MPI_Aint extent, lb=0;
    void *tmp_buf=NULL;
    int *displs=NULL;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm;

    rank = comm_ptr->rank;
    comm = comm_ptr->handle;
    local_size = comm_ptr->local_size;

    total_count = 0;
    for (i=0; i<local_size; i++) total_count += recvcnts[i];

    if (rank == 0) {
        /* In each group, rank 0 allocates a temp. buffer for the 
           reduce */

        displs = MPIU_Malloc(local_size*sizeof(int));
        if (!displs) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

        total_count = 0;
        for (i=0; i<local_size; i++) {
            displs[i] = total_count;
            total_count += recvcnts[i];
        }

        MPID_Datatype_get_extent_macro(datatype, extent);

        tmp_buf = MPIU_Malloc(extent*total_count);
        if (!tmp_buf) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        /* adjust for potential negative lower bound in datatype */
        /* MPI_Type_lb HAS NOT BEEN IMPLEMENTED YET. BUT lb IS
           INITIALIZED TO 0, AND DERIVED DATATYPES AREN'T SUPPORTED YET,
           SO IT'S OK */
#ifdef UNIMPLEMENTED
        MPI_Type_lb( datatype, &lb );
#endif
        tmp_buf = (void *)((char*)tmp_buf - lb);
    }

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
#ifdef UNIMPLEMENTED
    inleftgroup = yes_or_no;  /* not done */
#endif
    if (inleftgroup) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr);  
        if (mpi_errno) return mpi_errno;
    }


#ifdef UNIMPLEMENTED
    NMPI_Comm_group(comm, &group);
    MPID_Comm_return_intra(group, &newcomm);
#endif
    MPID_Comm_get_ptr( newcomm, newcomm_ptr );

    mpi_errno = MPIR_Scatterv(tmp_buf, recvcnts, displs, datatype, recvbuf,
                              recvcnts[rank], datatype, 0, newcomm_ptr);
    if (mpi_errno) return mpi_errno;
    
    if (rank == 0) {
        MPIU_Free(displs);
        MPIU_Free((char*)tmp_buf+lb);
    }

    return mpi_errno;

}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce_scatter

/*@
   MPI_Reduce_scatter - reduce scatter

   Arguments:
+  void *sendbuf - send buffer
.  void *recvbuf - receive buffer
.  int *recvcnts - receive counts
.  MPI_Datatype datatype - datatype
.  MPI_Op op - operation
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Reduce_scatter";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REDUCE_SCATTER);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_REDUCE_SCATTER);

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
            int rank;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE_SCATTER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

            rank = comm_ptr->rank;
	    MPIR_ERRTEST_COUNT(recvcnts[rank], mpi_errno);
	    MPIR_ERRTEST_DATATYPE(recvcnts[rank], datatype, mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE_SCATTER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Reduce_scatter != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Reduce_scatter(sendbuf, recvbuf,
                                                       recvcnts, datatype, 
                                                       op, comm_ptr);
    }
    else
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM) 
            /* intracommunicator */
            mpi_errno = MPIR_Reduce_scatter(sendbuf, recvbuf,
                                            recvcnts, datatype, 
                                            op, comm_ptr);
        else {
            /* intercommunicator */
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );
            /*mpi_errno = MPIR_Reduce_scatter_inter(sendbuf, recvbuf,
                                                  recvcnts, datatype, 
                                                  op, comm_ptr);           */
        }
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE_SCATTER);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE_SCATTER);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
