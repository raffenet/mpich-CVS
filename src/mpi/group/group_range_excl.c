/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_range_excl */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_range_excl = PMPI_Group_range_excl
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_range_excl  MPI_Group_range_excl
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_range_excl as PMPI_Group_range_excl
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_range_excl PMPI_Group_range_excl

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_range_excl

/*@
   MPI_Group_range_excl - group_range_excl

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
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup)
{
    static const char FCNAME[] = "MPI_Group_range_excl";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL, *new_group_ptr;
    int size, i, j, k, nnew, first, last, stride;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_RANGE_EXCL);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_RANGE_EXCL);
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

	    /* Check the exclusion array.  Ensure that all ranges are
	       valid and that the specified exclusions are unique */
	    if (group_ptr) {
		size = group_ptr->size;
		mpi_errno = MPIR_Group_check_valid_ranges( group_ptr, 
							   ranges, n );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_EXCL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Compute size, assuming that included ranks are valid (and distinct) */
    size = group_ptr->size;
    nnew = 0;
    for (i=0; i<n; i++) {
	first = ranges[i][0]; last = ranges[i][1]; stride = ranges[i][2];
	/* works for stride of either sign.  Error checking above 
	   has already guaranteed stride != 0 */
	nnew += 1 + (last - first) / stride;
    }
    nnew = size - nnew;

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create( nnew, &new_group_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_EXCL);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    new_group_ptr->rank = MPI_UNDEFINED;

    /* Group members are taken in rank order from the original group,
       with the specified members removed. Use the flag array for that
       purpose.  If this was a critical routine, we could use the 
       flag values set in the error checking part, if the error checking 
       was enabled *and* we are not MPI_THREAD_MULTIPLE, but since this
       is a low-usage routine, we haven't taken that optimization.  */

    MPID_Common_thread_lock();
    {
	/* First, mark the members to exclude */
	for (i=0; i<size; i++) 
	    group_ptr->lrank_to_lpid[i].flag = 0;
	
	for (i=0; i<n; i++) {
	    first = ranges[i][0]; last = ranges[i][1]; stride = ranges[i][2];
	    if (stride > 0) {
		for (j=first; j<=last; j += stride) {
		    group_ptr->lrank_to_lpid[j].flag = 1;
		}
	    }
	    else {
		for (j=first; j>=last; j += stride) {
		    group_ptr->lrank_to_lpid[j].flag = 1;
		}
	    }
	}
	/* Now, run through the group and pick up the members that were 
	   not excluded */
	k = 0;
	for (i=0; i<size; i++) {
	    if (!group_ptr->lrank_to_lpid[i].flag) {
		new_group_ptr->lrank_to_lpid[k].lrank = k;
		new_group_ptr->lrank_to_lpid[k].lpid = 
		    group_ptr->lrank_to_lpid[i].lpid;
		if (group_ptr->rank == i) {
		    new_group_ptr->rank = k;
		}
		k++;
	    }
	}

    }
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_RANGE_EXCL);
    return MPI_SUCCESS;
}

