/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_range_incl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_range_incl = PMPI_Group_range_incl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_range_incl  MPI_Group_range_incl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_range_incl as PMPI_Group_range_incl
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_range_incl PMPI_Group_range_incl

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_range_incl

/*@
   MPI_Group_range_incl - group_range_incl

   Arguments:
+  MPI_Group group - group
.  int n - n
.  int ranges[][3] - ranges
-  MPI_Group *newgroup - new group

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
EXPORT_MPI_API int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_range_incl";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_RANGE_INCL);
    /* Get handles to MPI objects. */
    MPID_Group_get_ptr( group, group_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_INCL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_INCL);
    return MPI_SUCCESS;
}

