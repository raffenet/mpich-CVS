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
                                       /* MPIR_MINLOC, MPIR_MAXLOC */ };

/* This is the default implementation of allreduce. The algorithm is:
   
   Algorithm: MPI_Allreduce

   For the homogeneous case, we use a recursive doubling algorithm
   (similar to the one in MPI_Allgather) for both short and long messages.
   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

   For the heterogeneous case, we call MPI_Reduce followed by MPI_Bcast
   in order to meet the requirement that all processes must have the
   same result.

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
    int rc, is_homogeneous;
    int        comm_size, rank;
    int        mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    int mask, dst, dst_tree_root, my_tree_root, nprocs_completed, k, i,
        j, tmp_mask, tree_root, is_commutative; 
    MPI_Aint extent, lb=0;
    void *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        printf("ERROR: MPI_Allreduce for intercommunicators not yet implemented.\n");
        NMPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    comm = comm_ptr->handle;
    
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    
    if (!is_homogeneous) {
        /* heterogeneous. To get the same result on all processes, we
           do a reduce to 0 and then broadcast. */
        mpi_errno = NMPI_Reduce ( sendbuf, recvbuf, count, datatype,
                                  op, 0, comm );
        if (mpi_errno == MPI_ERR_OP || mpi_errno == MPI_SUCCESS) {
            rc = NMPI_Bcast  ( recvbuf, count, datatype, 0, comm );
            if (rc) mpi_errno = rc;
        }
    }
    else {
        /* homogeneous. Use recursive doubling algorithm similar to the
           one used in all_gather */
        
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
            
            if ((op_ptr->language == MPID_LANG_C) || (op_ptr->language ==
                                                      MPID_LANG_CXX)) 
                uop = (MPI_User_function *) op_ptr->function.c_function;
            else
                uop = (MPI_User_function *) op_ptr->function.f77_function;
        }
        
        /* need to allocate temporary buffer to store incoming data*/
        MPID_Datatype_get_extent_macro(datatype, extent);
        tmp_buf = MPIU_Malloc(count*extent);
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
        
        /* Lock for collective operation */
        MPID_Comm_thread_lock( comm_ptr );
        
        /* copy local data into recvbuf */
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
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
                    (*uop)(tmp_buf, recvbuf, &count, &datatype);
                }
                else {
                    /* op is noncommutative and the order is not right */
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
        
        MPIU_Free((char *)tmp_buf+lb); 
        
        /* Unlock for collective operation */
        MPID_Comm_thread_unlock( comm_ptr );
    }
    
    return (mpi_errno);
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allreduce

/*@
MPI_Allreduce - Combines values from all processes and distributes the result
                back to all processes

Input Arguments:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Argument:
. recvbuf - starting address of receive buffer (choice) 

.N fortran

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
    MPID_Op *op_ptr = NULL;
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
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
	    MPID_Datatype_get_ptr(datatype, datatype_ptr);
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

            /*  MPID_Op_get_ptr(op, op_ptr);
            if (op_ptr == NULL) printf("error\n");
            MPID_Op_valid_ptr( op_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                } */
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
	mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype,
                                   op, comm_ptr); 
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

    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
    return MPI_SUCCESS;
}
