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
    MPID_File *file_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FILE_SET_ERRHANDLER);
    /* Get handles to MPI objects. */
    MPID_File_get_ptr( file, file_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate file_ptr */
            MPID_File_valid_ptr( file_ptr, mpi_errno );
	    /* If file_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_SET_ERRHANDLER);
                return MPIR_Err_return_file( file_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FILE_SET_ERRHANDLER);
    return MPI_SUCCESS;
}

