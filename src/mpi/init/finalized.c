/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Finalized */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Finalized = PMPI_Finalized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Finalized  MPI_Finalized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Finalized as PMPI_Finalized
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Finalized PMPI_Finalized
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Finalized

/*@
   MPI_Finalized - Indicates whether 'MPI_Finalize' has been called.

Output Argument:
. flag - Flag is true if 'MPI_Finalize' has been called and false otherwise. 

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Finalized( int * flag )
{
    static const char FCNAME[] = "MPI_Finalized";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FINALIZED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FINALIZED);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FINALIZED);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *flag = (MPIR_Process.initialized >= MPICH_POST_FINALIZED);
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FINALIZED);
    return MPI_SUCCESS;
}
