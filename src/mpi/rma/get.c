/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get = PMPI_Get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get  MPI_Get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get as PMPI_Get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Get PMPI_Get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get

/*@
   MPI_Get - Get data from a remote process

 Input Parameters:
+ origin_addr - Address of the buffer in which to receive the data
. origin_count - number of entries in origin buffer (nonnegative integer) 
. origin_datatype - datatype of each entry in origin buffer (handle) 
. target_rank - rank of target (nonnegative integer) 
. target_disp - displacement from window start to the beginning of the 
  target buffer (nonnegative integer) 
. target_count - number of entries in target buffer (nonnegative integer) 
. target_datatype - datatype of each entry in target buffer (handle) 
- win - window object used for communication (handle) 

 Output Parameter:
. origin_addr - initial address of origin buffer (choice) 

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_TYPE
.N MPI_ERR_WIN
@*/
int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win
            win)
{
    static const char FCNAME[] = "MPI_Get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET);

    MPID_MPI_RMA_FUNC_ENTER(MPID_STATE_MPI_GET);

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
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_GET);
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

            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_GET);
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
    mpi_errno = MPID_Get(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win_ptr);

    if (!mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_get", "**mpi_get %p %d %D %d %d %d %D %W", 
	    origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
	MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_GET);
	return MPI_SUCCESS;
    }
    
    MPID_MPI_RMA_FUNC_EXIT(MPID_STATE_MPI_GET);
    return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
}

