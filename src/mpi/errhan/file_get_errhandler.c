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

/* -- Begin Profiling Symbol Block for routine MPI_File_get_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_get_errhandler = PMPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_get_errhandler  MPI_File_get_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_get_errhandler as PMPI_File_get_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_File_get_errhandler PMPI_File_get_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_File_get_errhandler

/*@
   MPI_File_get_errhandler - get file error handler

   Arguments:
+  MPI_File file - file
-  MPI_Errhandler *errhandler - error handler

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_File_get_errhandler";
    int mpi_errno = MPI_SUCCESS;
#ifndef USE_ROMIO_FILE
    MPID_File *file_ptr = NULL;
#endif
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FILE_GET_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_GET_ERRHANDLER);

    /* Get handles to MPI objects. */
#ifndef USE_ROMIO_FILE
    MPID_File_get_ptr( file, file_ptr );
#endif
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(errhandler,"errhandler",mpi_errno);
#ifndef USE_ROMIO_FILE
            /* Validate file_ptr */
            MPID_File_valid_ptr( file_ptr, mpi_errno );
	    /* If file_ptr is not valid, it will be reset to null */
#endif
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
#ifdef USE_ROMIO_FILE
 {
     MPI_Errhandler eh;
     MPID_Errhandler *e;
     MPIR_ROMIO_Get_file_errhand( file, &eh );
     if (!eh) {
	 MPID_Errhandler_get_ptr( MPI_ERRORS_RETURN, e );
     }
     else {
	 MPID_Errhandler_get_ptr( eh, e );
     }
     MPIU_Object_add_ref( e );
     *errhandler = e->handle;
 }
#else
    /* Check for default error handler */
    if (!file_ptr->errhandler) {
	*errhandler = MPI_ERRORS_RETURN;
    }
    else {
	*errhandler = file_ptr->errhandler->handle;
	MPIU_Object_add_ref(file_ptr->errhandler);
    }
#endif
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_GET_ERRHANDLER);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_file_get_errhandler", "**mpi_file_get_errhandler %F %p", file, errhandler);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_GET_ERRHANDLER);
#ifdef USE_ROMIO_FILE
    return MPIR_Err_return_file( file, FCNAME, mpi_errno );
#else
    return MPIR_Err_return_file( file_ptr, FCNAME, mpi_errno );
#endif
    /* --END ERROR HANDLING-- */
}

