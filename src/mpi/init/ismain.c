/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Is_thread_main */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Is_thread_main = PMPI_Is_thread_main
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Is_thread_main  MPI_Is_thread_main
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Is_thread_main as PMPI_Is_thread_main
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Is_thread_main PMPI_Is_thread_main
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Is_thread_main

/*@
   MPI_Is_thread_main - Returns a flag indicating whether this thread called 
                        'MPI_Init' or 'MPI_Init_thread'

   Output Arguments:
. flag - Flag is true if 'MPI_Init' or 'MPI_Init_thread' has been called by 
         this thread and false otherwise.  

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Is_thread_main( int *flag )
{
    static const char FCNAME[] = "MPI_Is_thread_main";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_IS_THREAD_MAIN);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_IS_THREAD_MAIN);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
#if MPID_MAX_THREAD_LEVEL <= MPI_THREAD_FUNNELED
    *flag = 1;
#else
    *flag = (MPIR_Process.master_thread == MPID_GetThreadId());
#endif
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_IS_THREAD_MAIN);
    return MPI_SUCCESS;
}
