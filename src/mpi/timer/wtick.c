/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "timerconf.h"

/* -- Begin Profiling Symbol Block for routine MPI_Wtick */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wtick = PMPI_Wtick
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wtick  MPI_Wtick
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wtick as PMPI_Wtick
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Wtick PMPI_Wtick

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Wtick

/*@
  MPI_Wtick - Returns the frequency of Wtime

  Return value:
  Time in frequency of the values returned by MPI_Wtime

  Notes:

.see also: MPI_Wtime, MPI_Comm_get_attr, MPI_Attr_get
@*/
double MPI_Wtick( void )
{
    static const char FCNAME[] = "MPI_Wtick";
    int mpi_errno = MPI_SUCCESS;
    double tick;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WTICK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WTICK);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WTICK);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    tick = MPID_Wtick();

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WTICK);
    return tick;
}
