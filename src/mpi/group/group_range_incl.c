/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

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
int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_range_incl";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL, *new_group_ptr;
    int first, last, stride, nnew, i, j, k;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_RANGE_INCL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_RANGE_INCL);
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
	    if (group_ptr) {
		mpi_errno = MPIR_Group_check_valid_ranges( group_ptr, 
							   ranges, n );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_INCL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Compute size, assuming that included ranks are valid (and distinct) */
    nnew = 0;
    for (i=0; i<n; i++) {
	first = ranges[i][0]; last = ranges[i][1]; stride = ranges[i][2];
	/* works for stride of either sign.  Error checking above 
	   has already guaranteed stride != 0 */
	nnew += 1 + (last - first) / stride;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create( nnew, &new_group_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_INCL);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    new_group_ptr->rank = MPI_UNDEFINED;

    /* Group members taken in order specified by the range array */
    /* This could be integrated with the error checking, but since this
       is a low-usage routine, we haven't taken that optimization */
    k = 0;
    for (i=0; i<n; i++) {
	first = ranges[i][0]; last = ranges[i][1]; stride = ranges[i][2];
	if (stride > 0) {
	    for (j=first; j<=last; j += stride) {
		new_group_ptr->lrank_to_lpid[k].lrank = k;
		new_group_ptr->lrank_to_lpid[k].lpid = 
		    group_ptr->lrank_to_lpid[j].lpid;
		if (j == group_ptr->rank) 
		    new_group_ptr->rank = k;
		k++;
	    }
	}
	else {
	    for (j=first; j>=last; j += stride) {
		new_group_ptr->lrank_to_lpid[k].lrank = k;
		new_group_ptr->lrank_to_lpid[k].lpid = 
		    group_ptr->lrank_to_lpid[j].lpid;
		if (j == group_ptr->rank) 
		    new_group_ptr->rank = k;
		k++;
	    }
	}
    }
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_INCL);
    return MPI_SUCCESS;
}

