/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_free = PMPI_Errhandler_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_free  MPI_Errhandler_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_free as PMPI_Errhandler_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Errhandler_free PMPI_Errhandler_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Errhandler_free

/*@
  MPI_Errhandler_free - Frees an MPI-style errorhandler

Input Parameter:
. errhandler - MPI error handler (handle).  Set to 'MPI_ERRHANDLER_NULL' on 
exit.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Errhandler_free(MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Errhandler_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Errhandler *errhan_ptr = NULL;
    int in_use;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_FREE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERRHANDLER_FREE);

    MPID_Errhandler_get_ptr( *errhandler, errhan_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    
	    MPID_Errhandler_valid_ptr( errhan_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIU_Object_release_ref( errhan_ptr,&in_use);
    if (!in_use) {
	MPIU_Handle_obj_free( &MPID_Errhandler_mem, errhan_ptr );
    }
    *errhandler = MPI_ERRHANDLER_NULL;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_FREE);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_errhandler_free", "**mpi_errhandler_free %p", errhandler);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_FREE);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}

