/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Scan */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Scan = PMPI_Scan
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Scan  MPI_Scan
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Scan as PMPI_Scan
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Scan PMPI_Scan

/* This is the default implementation of scan. The algorithm is:
   
   Algorithm: MPI_Scan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

   recvbuf = sendbuf;
   partial_scan = sendbuf;
   mask = 0x1;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
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

   End Algorithm: MPI_Scan
*/


PMPI_LOCAL int MPIR_Scan ( 
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
    MPI_Aint extent, lb=0;
    void *partial_scan, *tmp_buf;
    MPI_User_function *uop;
    MPID_Op *op_ptr;
    MPI_Comm comm;
    MPICH_PerThread_t *p;
    
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
        if ((op_ptr->language == MPID_LANG_C) || (op_ptr->language ==
                                                  MPID_LANG_CXX)) 
#else
	if ((op_ptr->language == MPID_LANG_C))
#endif
            uop = (MPI_User_function *) op_ptr->function.c_function;
        else
            uop = (MPI_User_function *) op_ptr->function.f77_function;
    }
    
    /* need to allocate temporary buffer to store partial scan*/
    MPID_Datatype_get_extent_macro(datatype, extent);
    partial_scan = MPIU_Malloc(extent*count);
    if (!partial_scan) {
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
    partial_scan = (void *)((char*)partial_scan - lb);
    
    /* need to allocate temporary buffer to store incoming data*/
    tmp_buf = MPIU_Malloc(extent*count);
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
    
    /* Since this is an inclusive scan, copy local contribution into
       recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
        if (mpi_errno) return mpi_errno;
    }
    
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   partial_scan, count, datatype);
    else 
        mpi_errno = MPIR_Localcopy(recvbuf, count, datatype,
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
                                      dst, MPIR_SCAN_TAG, tmp_buf,
                                      count, datatype, dst,
                                      MPIR_SCAN_TAG, comm,
                                      &status); 
            if (mpi_errno) return mpi_errno;
            
            if (rank > dst) {
                (*uop)(tmp_buf, partial_scan, &count, &datatype);
                (*uop)(tmp_buf, recvbuf, &count, &datatype);
            }
            else {
                if (is_commutative)
                    (*uop)(tmp_buf, partial_scan, &count, &datatype);
                else {
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
    
    MPIU_Free((char *)partial_scan+lb); 
    MPIU_Free((char *)tmp_buf+lb); 
    
    /* Unlock for collective operation */
    MPID_Comm_thread_unlock( comm_ptr );
    
    if (p->op_errno) mpi_errno = p->op_errno;

    return (mpi_errno);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Scan

/*@
   MPI_Scan - scan

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
int MPI_Scan(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Scan";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SCAN);

    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_SCAN);

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
                MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }

            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(count, datatype, mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
                    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                }
            }
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr( op_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) {
                    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
                    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scan != NULL)
    {
	mpi_errno = comm_ptr->coll_fns->Scan(sendbuf, recvbuf, count,
                                             datatype, op, comm_ptr);
    }
    else
    {
	mpi_errno = MPIR_Scan(sendbuf, recvbuf, count, datatype,
                              op, comm_ptr); 
    }
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
	return MPI_SUCCESS;
    }
    else
    {
	MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCAN);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */
}
