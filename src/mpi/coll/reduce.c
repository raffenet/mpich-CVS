/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Reduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce = PMPI_Reduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce  MPI_Reduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce as PMPI_Reduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Reduce PMPI_Reduce

/* This is the default implementation of reduce. The algorithm is:
   
   Algorithm: MPI_Reduce

   We use a minimum spanning tree (MST) algorithm for both short and
   long messages. 

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

   Possible improvements: 

   End Algorithm: MPI_Reduce
*/

/* not declared static because it is called in intercomm. allreduce */
/* begin:nested */
int MPIR_Reduce ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    int root, 
    MPID_Comm *comm_ptr )
{
    MPI_Status status;
    int        comm_size, rank, is_commutative;
    int        mask, relrank, source, lroot;
    int        mpi_errno = MPI_SUCCESS;
    MPI_User_function *uop;
    MPI_Aint   true_lb, true_extent; 
    void       *buffer;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPICH_PerThread_t *p;
#ifdef HAVE_CXX_BINDING
    int is_cxx_uop = 0;
#endif
    
    if (count == 0) return MPI_SUCCESS;
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    /* set op_errno to 0. stored in perthread structure */
    MPID_GetPerThread(p);
    p->op_errno = 0;

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
    
    /* This code is from MPICH-1. */

    /* Here's the algorithm.  Relative to the root, look at the bit pattern in 
       my rank.  Starting from the right (lsb), if the bit is 1, send to 
       the node with that bit zero and exit; if the bit is 0, receive from the
       node with that bit set and combine (as long as that node is within the
       group)
       
       Note that by receiving with source selection, we guarentee that we get
       the same bits with the same input.  If we allowed the parent to receive 
       the children in any order, then timing differences could cause different
       results (roundoff error, over/underflows in some cases, etc).
       
       Because of the way these are ordered, if root is 0, then this is correct
       for both commutative and non-commutitive operations.  If root is not
       0, then for non-commutitive, we use a root of zero and then send
       the result to the root.  To see this, note that the ordering is
       mask = 1: (ab)(cd)(ef)(gh)            (odds send to evens)
       mask = 2: ((ab)(cd))((ef)(gh))        (3,6 send to 0,4)
       mask = 4: (((ab)(cd))((ef)(gh)))      (4 sends to 0)
       
       Comments on buffering.  
       If the datatype is not contiguous, we still need to pass contiguous 
       data to the user routine.  
       In this case, we should make a copy of the data in some format, 
       and send/operate on that.
       
       In general, we can't use MPI_PACK, because the alignment of that
       is rather vague, and the data may not be re-usable.  What we actually
       need is a "squeeze" operation that removes the skips.
    */
    /* Make a temporary buffer */

    mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb, &true_extent);  
    if (mpi_errno) return mpi_errno;

    buffer = MPIU_Malloc(true_extent * count);
    if (!buffer) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    /* adjust for potential negative lower bound in datatype */
    buffer = (void *)((char*)buffer - true_lb);
    
    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
        recvbuf = MPIU_Malloc(true_extent * count);
        if (!recvbuf) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
        if (mpi_errno) return mpi_errno;
    }

    mask    = 0x1;
    if (is_commutative) 
        lroot   = root;
    else
        lroot   = 0;
    relrank = (rank - lroot + comm_size) % comm_size;
    
    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );
    
    while (/*(mask & relrank) == 0 && */mask < comm_size) {
	/* Receive */
	if ((mask & relrank) == 0) {
	    source = (relrank | mask);
	    if (source < comm_size) {
		source = (source + lroot) % comm_size;
		mpi_errno = MPIC_Recv (buffer, count, datatype, source, 
				      MPIR_REDUCE_TAG, comm, &status);
		if (mpi_errno) return mpi_errno;
		/* The sender is above us, so the received buffer must be
		   the second argument (in the noncommutative case). */
		if (is_commutative) {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			MPIR_Call_op_fn( buffer, recvbuf, 
					 count, datatype, uop );
		    }
		    else 
#endif
		    (*uop)(buffer, recvbuf, &count, &datatype);
		}
		else {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			MPIR_Call_op_fn( recvbuf, buffer,
					 count, datatype, uop );
		    }
		    else 
#endif
		    (*uop)(recvbuf, buffer, &count, &datatype);
                    mpi_errno = MPIR_Localcopy(buffer, count, datatype,
                                               recvbuf, count, datatype);
                    if (mpi_errno) return mpi_errno;
                }
            }
        }
	else {
	    /* I've received all that I'm going to.  Send my result to 
	       my parent */
	    source = ((relrank & (~ mask)) + lroot) % comm_size;
	    mpi_errno  = MPIC_Send( recvbuf, count, datatype, 
                                    source, MPIR_REDUCE_TAG, comm );
	    if (mpi_errno) return mpi_errno;
	    break;
        }
	mask <<= 1;
    }

    MPIU_Free( (char *)buffer + true_lb );

    if (!is_commutative && (root != 0)) {
        if (rank == 0) {
            mpi_errno  = MPIC_Send( recvbuf, count, datatype, root, 
                                    MPIR_REDUCE_TAG, comm );
        }
        else if (rank == root) {
            mpi_errno = MPIC_Recv ( recvbuf, count, datatype, 0, 
                                   MPIR_REDUCE_TAG, comm, &status);
        }
        if (mpi_errno) return mpi_errno;
    }
    
    /* Free the temporarily allocated recvbuf */
    if (rank != root)
        MPIU_Free( (char *)recvbuf + true_lb );
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );

    if (p->op_errno) mpi_errno = p->op_errno;

    return (mpi_errno);
}
/* end:nested */

/* begin:nested */
PMPI_LOCAL int MPIR_Reduce_inter ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    int root, 
    MPID_Comm *comm_ptr )
{
/*  Intercommunicator reduce.
    Remote group does a local intracommunicator
    reduce to rank 0. Rank 0 then sends data to root.

    Cost: (lgp+1).alpha + n.(lgp+1).beta
*/

    int rank, mpi_errno;
    MPI_Status status;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf=NULL;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }
    
    comm = comm_ptr->handle;

    if (root == MPI_ROOT) {
            /* root receives data from rank 0 on remote group */
        MPID_Comm_thread_lock( comm_ptr );
        mpi_errno = MPIC_Recv(recvbuf, count, datatype, 0,
                              MPIR_REDUCE_TAG, comm, &status);
        MPID_Comm_thread_unlock( comm_ptr ); 
        
        return mpi_errno;
    }
    else {
        /* remote group. Rank 0 allocates temporary buffer, does
           local intracommunicator gather, and then sends the data
           to root. */
        
        rank = comm_ptr->rank;
        
        if (rank == 0) {
            mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                                  &true_extent);  
            if (mpi_errno) return mpi_errno;

            tmp_buf = MPIU_Malloc(true_extent*count);
            if (!tmp_buf) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
                return mpi_errno;
            }
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);
        }
        
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPIR_Setup_intercomm_localcomm( comm_ptr );

        newcomm_ptr = comm_ptr->local_comm;
        
        /* now do a local reduce on this intracommunicator */
        mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype,
                                op, 0, newcomm_ptr);
        if (mpi_errno) return mpi_errno;
        if (rank == 0) {
            MPID_Comm_thread_lock( comm_ptr );
            mpi_errno = MPIC_Send(tmp_buf, count, datatype, root,
                                  MPIR_REDUCE_TAG, comm); 
            MPID_Comm_thread_unlock( comm_ptr ); 
            if (mpi_errno) return mpi_errno;
            MPIU_Free((char*)tmp_buf+true_lb);
        }
    }
    
    return mpi_errno;
}
/* end:nested */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce

/*@
   MPI_Reduce - short description

   Arguments:
+  void *sendbuf - send buffer
.  void *recvbuf - receive buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  MPI_Op op - operation
.  int root - root
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Reduce";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REDUCE);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_REDUCE);

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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    /* FIXME: intra root only for MPI-1 */
	    MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Reduce != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Reduce(sendbuf, recvbuf, count,
                                               datatype, op, root, comm_ptr);
    }
    else
    {
	MPIR_Nest_incr();
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
            /* intracommunicator */
            mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype,
                                    op, root, comm_ptr); 
	}
        else {
            /* intercommunicator */
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_COMM, 
					      "**intercommcoll",
					      "**intercommcoll %s", FCNAME );
            /*mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype,
	      op, root, comm_ptr); */
        }
	MPIR_Nest_decr();
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_REDUCE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */

}
