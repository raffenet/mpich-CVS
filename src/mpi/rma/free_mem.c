/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Free_mem */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Free_mem = PMPI_Free_mem
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Free_mem  MPI_Free_mem
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Free_mem as PMPI_Free_mem
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Free_mem PMPI_Free_mem

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Free_mem

/*@
   MPI_Free_mem - free memory

   Arguments:
.  void *base - base

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Free_mem(void *base)
{
    static const char FCNAME[] = "MPI_Free_mem";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FREE_MEM);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FREE_MEM);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FREE_MEM);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPIU_Free(base);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FREE_MEM);
    return MPI_SUCCESS;
}
