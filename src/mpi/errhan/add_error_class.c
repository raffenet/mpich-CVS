/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Add_error_class */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Add_error_class = PMPI_Add_error_class
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Add_error_class  MPI_Add_error_class
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Add_error_class as PMPI_Add_error_class
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Add_error_class PMPI_Add_error_class

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Add_error_class

/*@
   MPI_Add_error_class - add error class

   Output Parameter:
.  errorclass - New error class

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Add_error_class(int *errorclass)
{
    static const char FCNAME[] = "MPI_Add_error_class";
    int mpi_errno = MPI_SUCCESS;
    int new_class;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ADD_ERROR_CLASS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ADD_ERROR_CLASS);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

	    if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    new_class = MPIR_Err_add_class( 0 );
    /* --BEGIN ERROR HANDLING-- */
    if (new_class < 0) {
	/* Error return.  */
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**noerrclasses", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    *errorclass = ERROR_DYN_MASK | new_class;
    MPIR_Setmax( &MPIR_Process.attrs.lastusedcode, *errorclass );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_CLASS);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_add_error_class", "**mpi_add_error_class %p", errorclass);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_CLASS);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

