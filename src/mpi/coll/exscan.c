/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Exscan */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Exscan = PMPI_Exscan
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Exscan  MPI_Exscan
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Exscan as PMPI_Exscan
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Exscan PMPI_Exscan

/* This is the default implementation of exscan. The algorithm is:
   
   Algorithm: MPI_Exscan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

   partial_scan = sendbuf;
   mask = 0x1;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
            if ((rank==1) && (dst==0))
               recv_buf = tmp_buf;
            else if (rank != 0)
               recvbuf = tmp_buf + recvbuf;
         }
         else {
            if (op is commutative)
               partial_scan = tmp_buf + partial_scan;
            else {
               tmp_buf = partial_scan + tmp_buf;
               partial_scan = tmp_buf;
            }
         }
      }
      mask <<= 1;
   }  

   End Algorithm: MPI_Exscan
*/

/* begin:nested */
PMPI_LOCAL int MPIR_Exscan ( 
    void *sendbuf, 
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPID_Comm *comm_ptr )
{
    MPI_Status status;
    int        rank, comm_size;
    int        mpi_errno = MPI_SUCCESS;
    int mask, dst, is_commutative; 
    MPI_Aint true_extent, true_lb;
    void *partial_scan, *tmp_buf;
    MPI_User_function *uop;
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
    
    /* need to allocate temporary buffer to store partial scan*/
    mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                          &true_extent);  
    if (mpi_errno) return mpi_errno;
    partial_scan = MPIU_Malloc(true_extent*count);
    if (!partial_scan) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);
    
    /* need to allocate temporary buffer to store incoming data*/
    tmp_buf = MPIU_Malloc(true_extent*count);
    if (!tmp_buf) {
        mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
        return mpi_errno;
    }
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                              partial_scan, count, datatype);
    if (mpi_errno) return mpi_errno;

    /* Lock for collective operation */
    MPID_Comm_thread_lock( comm_ptr );

    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                      dst, MPIR_EXSCAN_TAG, tmp_buf,
                                      count, datatype, dst,
                                      MPIR_EXSCAN_TAG, comm,
                                      &status); 
            if (mpi_errno) return mpi_errno;
            
            if (rank > dst) {
#ifdef HAVE_CXX_BINDING
		if (is_cxx_uop) {
		    MPIR_Call_op_fn( tmp_buf, partial_scan, 
				     count, datatype, uop );
		}
		else 
#endif
                (*uop)(tmp_buf, partial_scan, &count, &datatype);
                /* On rank 0, recvbuf is not defined.
                   On rank 1, recvbuf is to be set equal to the value
                   in sendbuf on rank 0.
                   On others, recvbuf is the scan of values in the
                   sendbufs on lower ranks. */ 
                if ((rank == 1) && (dst == 0)) {
                    /* simply copy data recd from rank 0 into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               recvbuf, count, datatype);
                    if (mpi_errno) return mpi_errno;
                }
                else if (rank != 0) {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			MPIR_Call_op_fn( tmp_buf, recvbuf, 
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(tmp_buf, recvbuf, &count, &datatype);
		}
            }
            else {
                if (is_commutative) {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			MPIR_Call_op_fn( tmp_buf, partial_scan, 
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(tmp_buf, partial_scan, &count, &datatype);
		}
                else {
#ifdef HAVE_CXX_BINDING
		    if (is_cxx_uop) {
			MPIR_Call_op_fn( partial_scan, tmp_buf,
					 count, datatype, uop );
		    }
		    else 
#endif
                    (*uop)(partial_scan, tmp_buf, &count, &datatype);
                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               partial_scan,
                                               count, datatype);
                    if (mpi_errno) return mpi_errno;
                }
            }
        }
        mask <<= 1;
    }
    
    MPIU_Free((char *)partial_scan+true_lb); 
    MPIU_Free((char *)tmp_buf+true_lb); 
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    if (p->op_errno) mpi_errno = p->op_errno;

    return (mpi_errno);
}
/* end:nested */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Exscan

/*@
   MPI_Exscan - exscan

   Arguments:
+  void *sendbuf - send buffer
.  void *recvbuf - receive buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  MPI_Op op - operation
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Exscan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Exscan";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_EXSCAN);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_EXSCAN);

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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_EXSCAN);
                return MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
            }
            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_EXSCAN);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Exscan != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Exscan(sendbuf, recvbuf, count,
                                             datatype, op, comm_ptr);
    }
    else
    {
	MPIR_Nest_incr();
	mpi_errno = MPIR_Exscan(sendbuf, recvbuf, count, datatype,
                              op, comm_ptr); 
	MPIR_Nest_decr();
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_EXSCAN);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_EXSCAN);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
