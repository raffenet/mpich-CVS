/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Add_error_code */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Add_error_code = PMPI_Add_error_code
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Add_error_code  MPI_Add_error_code
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Add_error_code as PMPI_Add_error_code
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Add_error_code PMPI_Add_error_code

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Add_error_code

/*@
   MPI_Add_error_code - add error code

   Input Parameter:
.  errorclass - Error class to add an error code.

   Output Parameter:
.  errorcode - New error code for this error class.

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Add_error_code(int errorclass, int *errorcode)
{
    static const char FCNAME[] = "MPI_Add_error_code";
    int mpi_errno = MPI_SUCCESS;
    int new_code;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ADD_ERROR_CODE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ADD_ERROR_CODE);
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
    new_code = MPIR_Err_add_code( errorclass, 0 );
    /* --BEGIN ERROR HANDLING-- */
    if (new_code < 0) {
	/* Error return.  */
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**noerrcodes", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    *errorcode = new_code;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_CODE);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_add_error_code", "**mpi_add_error_code %d %p", errorclass, errorcode);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_CODE);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

