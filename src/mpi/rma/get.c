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
   MPI_Get - get

   Arguments:
+  void *origin_addr - origin address
.  int origin_count - origin count
.  MPI_Datatype origin_datatype - origin datatype
.  int target_rank - target rank
.  MPI_Aint target_disp - target disp
.  int target_count - target count
.  MPI_Datatype target_datatype - target datatype
-  MPI_Win win - window

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    static const char FCNAME[] = "MPI_Get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET);
    /* Get handles to MPI objects. */
    MPID_Win_get_ptr( win, win_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate win_ptr */
            MPID_Win_valid_ptr( win_ptr, mpi_errno );
	    /* If group_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET);
                return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Get(origin_addr, origin_count, origin_datatype,
                         target_rank, target_disp, target_count,
                         target_datatype, win_ptr);

    if (!mpi_errno)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET);
	return MPI_SUCCESS;
    }
    
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET);
    return MPIR_Err_return_win( win_ptr, FCNAME, mpi_errno );
}

