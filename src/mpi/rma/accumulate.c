/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Accumulate */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Accumulate = PMPI_Accumulate
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Accumulate  MPI_Accumulate
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Accumulate as PMPI_Accumulate
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Accumulate PMPI_Accumulate

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Accumulate

/*@
   MPI_Accumulate - accumulate

   Input Parameters:
+ origin_addr - initial address of buffer (choice) 
. origin_count - number of entries in buffer (nonnegative integer) 
. origin_datatype - datatype of each buffer entry (handle) 
. target_rank - rank of target (nonnegative integer) 
. target_disp - displacement from start of window to beginning of target 
  buffer (nonnegative integer)  
. target_count - number of entries in target buffer (nonnegative integer) 
. target_datatype - datatype of each entry in target buffer (handle) 
. op - predefined reduce operation (handle) 
- win - window object (handle) 

   Notes:
The basic components of both the origin and target datatype must be the same 
predefined datatype (e.g., all 'MPI_INT' or all 'MPI_DOUBLE_PRECISION').

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                   origin_datatype, int target_rank, MPI_Aint
                   target_disp, int target_count, MPI_Datatype
                   target_datatype, MPI_Op op, MPI_Win win) 
{
    static const char FCNAME[] = "MPI_Accumulate";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ACCUMULATE);

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_ACCUMULATE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno != MPI_SUCCESS) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            int comm_size;

            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);
                return MPIR_Err_return_win( NULL, FCNAME, mpi_errno );
            }

	    MPIR_ERRTEST_COUNT(origin_count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(origin_count, origin_datatype, mpi_errno);
	    MPIR_ERRTEST_COUNT(target_count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(target_count, target_datatype, mpi_errno);
	    MPIR_ERRTEST_DISP(target_disp, mpi_errno);

            MPIR_Nest_incr();
            NMPI_Comm_size(win_ptr->comm, &comm_size);
            MPIR_Nest_decr();
            if ((target_rank < MPI_PROC_NULL) || (target_rank >=
                                                  comm_size))
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_RANK,
                   "**rank", "**rank %d %d", target_rank, comm_size );

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**op", 0);

            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);
                return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (target_rank == MPI_PROC_NULL) return MPI_SUCCESS;

/*    if (HANDLE_GET_KIND(target_datatype) != HANDLE_KIND_BUILTIN) {
        printf("ERROR: RMA operations not supported for derived datatypes on target\n");
        NMPI_Abort(MPI_COMM_WORLD,1);
    }
*/
    mpi_errno = MPID_Accumulate(origin_addr, origin_count, origin_datatype,
                                target_rank, target_disp, target_count,
                                target_datatype, op, win_ptr);

    if (!mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_accumulate", "**mpi_accumulate %p %d %D %d %d %d %D %O %W",
	    origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, op, win);
	MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);
	return mpi_errno;
    }

    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);
    return MPI_SUCCESS;
}

