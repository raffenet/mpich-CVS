/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_free = PMPI_Info_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_free  MPI_Info_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_free as PMPI_Info_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_free PMPI_Info_free
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_free

/*@
    MPI_Info_free - Frees an info object

Input Parameter:
. info - info object (handle)

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_OTHER
@*/
int MPI_Info_free( MPI_Info *info )
{
    static const char FCNAME[] = "MPI_Info_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr=0;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_FREE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_FREE);
    /* Get handles to MPI objects. */
    MPID_Info_get_ptr( *info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIU_Info_free( info_ptr );
    *info = MPI_INFO_NULL;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_FREE);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_info_free", "**mpi_info_free %p", info);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_FREE);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
