/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Error_string */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Error_string = PMPI_Error_string
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Error_string  MPI_Error_string
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Error_string as PMPI_Error_string
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Error_string PMPI_Error_string

#endif

/* Forward reference for accessing error strings */
const char *MPIR_Err_get_generic_string( int class );

#undef FUNCNAME
#define FUNCNAME MPI_Error_string

/*@
   MPI_Error_string - error string

   Arguments:
+  int errorcode - error code
.  char *string - string
-  int *resultlen - result length

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Error_string(int errorcode, char *string, int *resultlen)
{
    static const char FCNAME[] = "MPI_Error_string";
    int mpi_errno = MPI_SUCCESS;
    const char *p;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERROR_STRING);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERROR_STRING);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_STRING);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* The assumption here is that the string remains valid until
       after the strncpy.  We could lock around the error message
       routines if necessary, but providing a pointer to the string
       simplifies the creation of error messages for the MPI_ERRORS_FATAL
       error handler */
    p = MPIR_Err_get_string( errorcode );
    *resultlen = strlen( p );
    MPIU_Strncpy( string, p, MPI_MAX_ERROR_STRING );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_STRING);
    return MPI_SUCCESS;
}

