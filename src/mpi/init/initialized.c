/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Initialized */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Initialized = PMPI_Initialized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Initialized  MPI_Initialized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Initialized as PMPI_Initialized
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Initialized PMPI_Initialized
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Initialized

/*@
   MPI_Initialized - Indicates whether 'MPI_Init' has been called.

Output Argument:
. flag - Flag is true if 'MPI_Init' or 'MPI_Init_thread' has been called and 
         false otherwise.  

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Initialized( int *flag )
{
    static const char FCNAME[] = "MPI_Initialized";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INITIALIZED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INITIALIZED);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Should check that flag is not null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INITIALIZED);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *flag = (MPIR_Process.initialized >= MPICH_WITHIN_MPI);
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INITIALIZED);
    return MPI_SUCCESS;
}
