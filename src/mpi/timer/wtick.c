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
  MPI_Wtick - Returns the resolution of MPI_Wtime

  Return value:
  Time in seconds of resolution of MPI_Wtime

  Notes for Fortran:
  This is a function, declared as 'DOUBLE PRECISION MPI_WTICK()' in Fortran.

.see also: MPI_Wtime, MPI_Comm_get_attr, MPI_Attr_get
@*/
double MPI_Wtick( void )
{
    static const char FCNAME[] = "MPI_Wtick";
    int mpi_errno = MPI_SUCCESS;
    double tick;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WTICK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_WTICK);
    /* The only possible error is MPI is not initialized, in 
       which case the error mechanism cannot be used. */
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    tick = MPID_Wtick();

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WTICK);
    return tick;
fn_fail:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_WTICK);
    /* There is no valid object to use for the communicator */
    MPID_Abort( 0, MPI_ERR_OTHER, MPI_ERR_OTHER, "" );
    return 0.0;
}
