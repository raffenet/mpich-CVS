/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_compare */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_compare = PMPI_Group_compare
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_compare  MPI_Group_compare
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_compare as PMPI_Group_compare
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Group_compare PMPI_Group_compare

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_compare

/*@
   MPI_Group_compare - compare two groups

   Arguments:
+  MPI_Group group1 - group1
.  MPI_Group group2 - group2
-  int *result - result

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result)
{
    static const char FCNAME[] = "MPI_Group_compare";
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr1 = NULL;
    MPID_Group *group_ptr2 = NULL;
    int g1_idx, g2_idx, size, i;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_COMPARE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_COMPARE);
    /* Get handles to MPI objects. */
    MPID_Group_get_ptr( group1, group_ptr1 );
    MPID_Group_get_ptr( group2, group_ptr2 );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr1, mpi_errno );
            MPID_Group_valid_ptr( group_ptr2, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_COMPARE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* See if their sizes are equal */
    if (group_ptr1->size != group_ptr2->size) {
	(*result) = MPI_UNEQUAL;
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_COMPARE);
	return MPI_SUCCESS;
    }

    /* Run through the lrank to lpid lists of each group in lpid order
       to see if the same processes are involved */
    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;
    /* If the lpid list hasn't been created, do it now */
    if (g1_idx < 0) { 
	MPIR_Group_setup_lpid_list( group_ptr1 ); 
	g1_idx = group_ptr1->idx_of_first_lpid;
    }
    if (g2_idx < 0) { 
	MPIR_Group_setup_lpid_list( group_ptr2 ); 
	g2_idx = group_ptr2->idx_of_first_lpid;
    }
    while (g1_idx >= 0 && g2_idx >= 0) {
	if (group_ptr1->lrank_to_lpid[g1_idx].lpid !=
	    group_ptr2->lrank_to_lpid[g2_idx].lpid) {
	    *result = MPI_UNEQUAL;
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_COMPARE);
	    return MPI_SUCCESS;
	}
	g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
	g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
    }

    /* See if the processes are in the same order by rank */
    size = group_ptr1->size;
    for (i=0; i<size; i++) {
	if (group_ptr1->lrank_to_lpid[i].lpid != 
	    group_ptr2->lrank_to_lpid[i].lpid) {
	    *result = MPI_SIMILAR;
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_COMPARE);
	    return MPI_SUCCESS;
	}
    }
	
    /* If we reach here, the groups are identical */
    *result = MPI_IDENT;
   
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_COMPARE);
    return MPI_SUCCESS;
}
