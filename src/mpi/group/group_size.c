/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_size = PMPI_Group_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_size  MPI_Group_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_size as PMPI_Group_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_size PMPI_Group_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_size

/*@
   MPI_Group_size - size of a group

   Arguments:
+  MPI_Group group - group
-  int *size - size

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Group_size(MPI_Group group, int *size)
{
    static const char FCNAME[] = "MPI_Group_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_SIZE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_SIZE);
    /* Get handles to MPI objects. */
    MPID_Group_get_ptr( group, group_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_SIZE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    *size = group_ptr->size;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_SIZE);
    return MPI_SUCCESS;
}

