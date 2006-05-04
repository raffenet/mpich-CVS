/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* mpiext.h contains the prototypes for functions to interface MPICH2
   and ROMIO */
#include "mpiext.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_call_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_call_errhandler = PMPI_File_call_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_call_errhandler  MPI_File_call_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_call_errhandler as PMPI_File_call_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_File_call_errhandler PMPI_File_call_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_File_call_errhandler
#undef FCNAME
#define FCNAME "MPI_File_call_errhander"
/*@
   MPI_File_call_errhandler - Call the error handler installed on a 
   file

   Input Parameters:
+ fh - MPI file with error handler (handle) 
- errorcode - error code (integer) 

.N ThreadSafeNoUpdate

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_FILE
@*/
int MPI_File_call_errhandler(MPI_File fh, int errorcode)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Errhandler *e;
    MPI_Errhandler eh;
    MPIU_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPIU_THREADPRIV_GET;

#ifdef MPI_MODE_RDONLY
    /* Validate parameters, especially handles needing to be converted */
    /* FIXME: check for a valid file handle (fh) before converting to a 
       pointer */
    
    /* ... body of routine ...  */
    
    MPIR_ROMIO_Get_file_errhand( fh, &eh );
    if (!eh) {
	MPID_Errhandler_get_ptr( MPI_ERRORS_RETURN, e );
    }
    else {
	MPID_Errhandler_get_ptr( eh, e );
    }

    /* The user error handler may make calls to MPI routines, so the nesting
     * counter must be incremented before the handler is called */
    MPIR_Nest_incr();
    
    switch (e->language) {
    case MPID_LANG_C:
	(*e->errfn.C_File_Handler_function)( &fh, &errorcode );
	break;
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
	/* See HAVE_LANGUAGE_FORTRAN below for an explanation */
	{ int fh1 = (int)fh;
	(*MPIR_Process.cxx_call_errfn)( 1, &fh1, &errorcode, 
			(void (*)(void))*e->errfn.C_File_Handler_function );
	}
	break;
#endif
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	/* The assignemt to a local variable prevents the compiler
	   from generating a warning about a type-punned pointer.  Since
	   the value is really const (but MPI didn't define error handlers 
	   with const), this preserves the intent */
	{ MPI_Fint fh1 = (MPI_Fint)fh;
	(*e->errfn.F77_Handler_function)( &fh1, &errorcode );
	}
	break;
#endif
    }

    MPIR_Nest_decr();
    
#else
    /* Dummy in case ROMIO is not defined */
    mpi_errno = MPI_ERR_INTERN;
#endif
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    return mpi_errno;

    /* ifdef out until we need it */
#if 0
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_file_call_errhandler",
	    "**mpi_file_call_errhandler %F %d", fh, errorcode);
    }
#   endif
#ifdef MPI_MODE_RDONLY
    mpi_errno = MPIO_Err_return_file( fh, mpi_errno );
#endif
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#endif
}
