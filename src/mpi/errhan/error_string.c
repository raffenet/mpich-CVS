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

/* This global is used to decouple the code for the predefined from the
   dynamically defined error codes and classes */
int (*MPIR_dnyErr_to_string)( int, char *, int ) = 0;
#endif

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
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERROR_STRING);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_STRING);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert the code to a string.  The cases are:
       simple class.  Find the corresponding string.
       <not done>
       if (user code) { go to code that extracts user error messages }
       else {
           is specific message code set and available?  if so, use it
	   else use generic code (lookup index in table of messages)
       }
     */
    if ( (errorcode && ERROR_CLASS_MASK) == errorcode) {
	/* code is a raw error class.  Convert the class to an index */
	/* class_to_index[errorcode] */
	/* Not correct yet, and this data is only available as static info in
	   errutil.c */
;
    }
    /* Not implemented */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERROR_STRING);
    return MPI_SUCCESS;
}

