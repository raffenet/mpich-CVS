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
  MPI_Pcontrol - Controls profiling

  Input Parameters:
+ level - Profiling level 
-  ... - other arguments (see notes)

  Notes:
  This routine provides a common interface for profiling control.  The
  interpretation of 'level' and any other arguments is left to the
  profiling library.  The intention is that a profiling library will 
  provide a replacement for this routine and define the interpretation
  of the parameters.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Pcontrol(const int level, ...)
{
    static const char FCNAME[] = "MPI_Pcontrol";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PCONTROL);

    MPIR_ERRTEST_INITIALIZED_ORRETURN();
    
    MPID_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PCONTROL);

    /* ... body of routine ...  */
    
    /* This is a dummy routine that does nothing.  It is intended for 
       use by the user (or a tool) with the profiling interface */
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PCONTROL);
    MPID_CS_EXIT();
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_pcontrol", "**mpi_pcontrol %d", level);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
