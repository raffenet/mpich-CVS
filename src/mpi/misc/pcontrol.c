/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pcontrol */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pcontrol = PMPI_Pcontrol
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pcontrol  MPI_Pcontrol
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pcontrol as PMPI_Pcontrol
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Pcontrol PMPI_Pcontrol

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pcontrol

/*@
   MPI_Pcontrol - profiling control

   Arguments:
+  const int level - level
-  ... - other arguments

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Pcontrol(const int level, ...)
{
    static const char FCNAME[] = "MPI_Pcontrol";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PCONTROL);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PCONTROL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* This is a dummy routine that does nothing.  It is intended for 
       use by the user (or a tool) with the profiling interface */
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PCONTROL);
    return MPI_SUCCESS;
}
