/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* -- Begin Profiling Symbol Block for routine MPI_Wtime */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Wtime = PMPI_Wtime
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Wtime  MPI_Wtime
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Wtime as PMPI_Wtime
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Wtime PMPI_Wtime
#endif

#include "timerconf.h"

/*@
  MPI_Wtime - Returns an elapsed time on the calling processor

  Return value:
  Time in seconds since an arbitrary time in the past.

  Notes:
  This is intended to be a high-resolution, elapsed (or wall) clock.
  See 'MPI_WTICK' to determine the resolution of 'MPI_WTIME'.
  If the attribute 'MPI_WTIME_IS_GLOBAL' is defined and true, then the
  value is synchronized across all processes in 'MPI_COMM_WORLD'.  

  Notes for Fortran:
  This is a function, declared as 'DOUBLE PRECISION MPI_WTIME()' in Fortran.

.see also: MPI_Wtick, MPI_Comm_get_attr, MPI_Attr_get
@*/
EXPORT_MPI_API double MPI_Wtime( void )
{
    double t1;
    MPID_Time_t t;

    MPID_Wtime( &t );
    MPID_Wtime_todouble( &t, &t1 );
    return t1;
}
