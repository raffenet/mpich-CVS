/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Bsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bsend = PMPI_Bsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bsend  MPI_Bsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bsend as PMPI_Bsend
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Bsend PMPI_Bsend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Bsend

/*@
   MPI_Bsend - bsend

   Arguments:
+  void *buf - buffer
.  int count - count
.  MPI_Datatype datatype - datatype
.  int dest - dest
.  int tag - tag
-  MPI_Comm comm - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Bsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
	      MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Bsend";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_BSEND);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count,mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr) {
		MPIR_ERRTEST_SEND_TAG(tag,mpi_errno);
		MPIR_ERRTEST_SEND_RANK(comm_ptr,tag,mpi_errno)
	    }

            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
#ifdef MPID_HAS_TBSEND
    mpi_errno = MPID_tBsend( buf, count, datatype, dest, tag, comm_ptr, 0 );
    if (mpi_errno == MPI_SUCCESS) {
	MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
	return MPI_SUCCESS;
    }
    /* Check for MPID_WOULD_BLOCK? */
#endif    
    
    /* ... end of body of routine ... */

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_BSEND);
    return MPI_SUCCESS;
}
