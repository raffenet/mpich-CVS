/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_free = PMPI_Group_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_free  MPI_Group_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_free as PMPI_Group_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_free PMPI_Group_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_free

/*@

MPI_Group_free - Frees a group

Input Parameter:
. group - group to free (handle) 

Notes:
On output, group is set to 'MPI_GROUP_NULL'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_PERM_GROUP
@*/
int MPI_Group_free(MPI_Group *group)
{
    static const char FCNAME[] = "MPI_Group_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_FREE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_FREE);
    /* Get handles to MPI objects. */
    MPID_Group_get_ptr( *group, group_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Do not free MPI_GROUP_EMPTY */
    if (*group != MPI_GROUP_EMPTY)
        mpi_errno = MPIR_Group_release(group_ptr);

    *group = MPI_GROUP_NULL;
    /* ... end of body of routine ... */
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_FREE);
	return MPI_SUCCESS;
    }
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_group_free", "**mpi_group_free %p", group);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_FREE);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
