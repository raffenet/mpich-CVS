/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_File_set_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_set_errhandler = PMPI_File_set_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_set_errhandler  MPI_File_set_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_set_errhandler as PMPI_File_set_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_File_set_errhandler PMPI_File_set_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_File_set_errhandler

/*@
   MPI_File_set_errhandler - set file error handler

   Arguments:
+  MPI_File file - file
-  MPI_Errhandler errhandler - error handler

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler)
{
    static const char FCNAME[] = "MPI_File_set_errhandler";
    int mpi_errno = MPI_SUCCESS;
#ifndef USE_ROMIO_FILE
    MPID_File *file_ptr = NULL;
#endif
    int in_use;
    MPID_Errhandler *errhan_ptr = NULL, *old_errhandler;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FILE_SET_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_SET_ERRHANDLER);

    /* Get handles to MPI objects. */
#ifndef USE_ROMIO_FILE
    MPID_File_get_ptr( file, file_ptr );
#endif
    MPID_Errhandler_get_ptr( errhandler, errhan_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
#ifndef USE_ROMIO_FILE
            /* Validate file_ptr */
            MPID_File_valid_ptr( file_ptr, mpi_errno );
	    /* If file_ptr is not valid, it will be reset to null */
#endif

	    if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
		MPID_Errhandler_valid_ptr( errhan_ptr,mpi_errno );
		/* Also check for a valid errhandler kind */
		if (!mpi_errno) {
		    if (errhan_ptr->kind != MPID_FILE) {
			mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**errhandnotfile", 0 );
		    }
		}
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_SET_ERRHANDLER);
#ifdef USE_ROMIO_FILE
                return MPIR_Err_return_file( file, FCNAME, mpi_errno );
#else
                return MPIR_Err_return_file( file_ptr, FCNAME, mpi_errno );
#endif
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
#ifdef USE_ROMIO_FILE
	MPIR_ROMIO_Get_file_errhand( file, &old_errhandler );
	if (!old_errhandler) {
	    MPID_Errhandler_get_ptr( MPI_ERRORS_RETURN, old_errhandler );
	}
#else
	old_errhandler = file_ptr->errhandler;
#endif
	if (old_errhandler) {
	    MPIU_Object_release_ref(old_errhandler,&in_use);
	    if (!in_use) {
		MPID_Errhandler_free( old_errhandler );
	    }
	}
    }

    MPIU_Object_add_ref(errhan_ptr);
#ifdef USE_ROMIO_FILE
    MPIR_ROMIO_Set_file_errhand( file, errhan_ptr );
#else
    file_ptr->errhandler = errhan_ptr;
#endif
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_SET_ERRHANDLER);
    return MPI_SUCCESS;
}

#ifndef MPICH_MPI_FROM_PMPI
void MPIR_Get_file_error_routine( MPID_Errhandler *e, 
				  void (**c)(MPI_File *, int *, ...), 
				   int *kind )
{
    if (!e) {
	*c = 0;
	*kind = 1; /* Use errors return as the default */
    }
    else {
	if (e->handle == MPI_ERRORS_RETURN) {
	    *c = 0;
	    *kind = 1;
	}
	else if (e->handle == MPI_ERRORS_ARE_FATAL) {
	    *c = 0;
	    *kind = 0;
	}
	else {
	    *c = e->errfn.C_File_Handler_function;
	    *kind = 2;
	}
    }
}
#endif
