/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

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
+ fh - file with error handler (handle) 
- errorcode - error code (integer) 

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
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    /* Get handles to MPI objects. */
    MPID_File_get_ptr( fh, file_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);

            /* Validate file_ptr */
            MPID_File_valid_ptr( file_ptr, mpi_errno );
	    /* If file_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
                return MPIR_Err_return_file( file_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    switch (file_ptr->errhandler->language) {
    case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
#endif
	(*file_ptr->errhandler->errfn.C_File_Handler_function)( 
	    &file_ptr->handle, &errorcode );
	break;
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	(*file_ptr->errhandler->errfn.F77_Handler_function)( 
	    (MPI_Fint *)&file_ptr->handle, &errorcode );
	break;
#endif
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_CALL_ERRHANDLER);
    return MPI_SUCCESS;
}
