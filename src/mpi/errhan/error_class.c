/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Error_class */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Error_class = PMPI_Error_class
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Error_class  MPI_Error_class
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Error_class as PMPI_Error_class
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Error_class PMPI_Error_class

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Error_class

/*@
   MPI_Error_class - error class

   Arguments:
+  int errorcode - error code
-  int *errorclass - error class

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Error_class(int errorcode, int *errorclass)
{
    static const char FCNAME[] = "MPI_Error_class";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERROR_CLASS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERROR_CLASS);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_CLASS);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *errorclass = errorcode & ERROR_CLASS_MASK;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_CLASS);
    return MPI_SUCCESS;
}

