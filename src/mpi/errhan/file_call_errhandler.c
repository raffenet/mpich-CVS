/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#ifdef USE_ROMIO_FILE
/* Forward ref for the routine to extract and set the error handler
   in a ROMIO File structure.  FIXME: These should be imported from a common
   header file that is also used in mpich2_fileutil.c
 */
int MPIR_ROMIO_Get_file_errhand( MPI_File, MPI_Errhandler * );
int MPIR_ROMIO_Set_file_errhand( MPI_File, MPI_Errhandler );
void MPIR_Get_file_error_routine( MPID_Errhandler *, 
				  void (**)(MPI_File *, int *, ...), 
				  int * );
#endif

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
    static const char FCNAME[] = "MPI_File_call_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_File *file_ptr = NULL;
    MPID_Errhandler *e;
    MPI_Errhandler eh;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    /* Get handles to MPI objects. */
#ifndef USE_ROMIO_FILE
    MPID_File_get_ptr( fh, file_ptr );
#endif
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);

#ifndef USE_ROMIO_FILE
            /* Validate file_ptr */
            MPID_File_valid_ptr( file_ptr, mpi_errno );
	    /* If file_ptr is not value, it will be reset to null */
#endif
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
#ifdef USE_ROMIO_FILE
 {
     MPIR_ROMIO_Get_file_errhand( fh, &eh );
     if (!eh) {
	 MPID_Errhandler_get_ptr( MPI_ERRORS_RETURN, e );
     }
     else {
	 MPID_Errhandler_get_ptr( eh, e );
     }
 }
#else
    e = file_ptr->errhandler;
#endif
    switch (e->language) {
    case MPID_LANG_C:
	(*e->errfn.C_File_Handler_function)( &fh, &errorcode );
	break;
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
	(*MPIR_Process.cxx_call_errfn)( 1, (int*)&fh, &errorcode, 
			(void (*)(void))*e->errfn.C_File_Handler_function );
	break;
#endif
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	(*e->errfn.F77_Handler_function)( (MPI_Fint *)&fh, &errorcode );
	break;
#endif
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_file_call_errhandler", "**mpi_file_call_errhandler %F %d", fh, errorcode);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
#ifdef USE_ROMIO_FILE
    return MPIO_Err_return_file( fh, mpi_errno );
#else
    return MPIR_Err_return_file( file_ptr, FCNAME, mpi_errno );
#endif
    /* --END ERROR HANDLING-- */
}
