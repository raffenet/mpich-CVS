/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_hindexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_hindexed = PMPI_Type_hindexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_hindexed  MPI_Type_hindexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_hindexed as PMPI_Type_hindexed
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_hindexed PMPI_Type_hindexed

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_hindexed

/*@
    MPI_Type_hindexed - Creates an indexed datatype with offsets in bytes

Input Parameters:
+ count - number of blocks -- also number of entries in indices and blocklens
. blocklens - number of elements in each block (array of nonnegative integers) 
. indices - byte displacement of each block (array of MPI_Aint) 
- old_type - old datatype (handle) 

Output Parameter:
. newtype - new datatype (handle) 

.N Fortran

Also see the discussion for 'MPI_Type_indexed' about the 'indices' in Fortran.

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_EXHAUSTED
.N MPI_ERR_ARG
@*/
int MPI_Type_hindexed(int count,
		      int blocklens[],
		      MPI_Aint indices[],
		      MPI_Datatype old_type,
		      MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_hindexed";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_HINDEXED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_HINDEXED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int i;
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE_NULL(old_type, "datatype", mpi_errno);
	    if (count > 0) {
		/* Only teset the others if the count is positive.
		   This focuses attention on count */
		MPIR_ERRTEST_ARGNULL(blocklens, "blocklens", mpi_errno);
		MPIR_ERRTEST_ARGNULL(indices, "indices", mpi_errno);
	    }
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype_get_ptr( old_type, datatype_ptr );
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		}
		/* verify that all blocklengths are > 0 (0 isn't ok is it?) */
		for (i=0; i < count; i++) MPIR_ERRTEST_ARGNEG(blocklens[i], "blocklen", mpi_errno);
	    }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_indexed(count,
				  blocklens,
				  indices,
				  1, /* displacements in bytes */
				  old_type,
				  newtype);

    if (mpi_errno == MPI_SUCCESS) {
	MPID_Datatype *new_dtp;
	int i, *ints;

	ints = (int *) MPIU_Malloc((count + 1) * sizeof(int));
	/* --BEGIN ERROR HANDLING-- */
	if (ints == NULL)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno,
					     MPIR_ERR_RECOVERABLE,
					     FCNAME,
					     __LINE__,
					     MPI_ERR_OTHER,
					     "**nomem",
					     "**nomem %d",
					     (count+1) * sizeof(int));
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */

	/* copy ints into temporary buffer (count and blocklengths) */
	ints[0] = count;
	for (i=0; i < count; i++) {
	    ints[i+1] = blocklens[i];
	}

	MPID_Datatype_get_ptr(*newtype, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       MPI_COMBINER_HINDEXED,
					       count+1, /* ints */
					       count, /* aints (displs) */
					       1, /* types */
					       ints,
					       indices,
					       &old_type);
	MPIU_Free(ints);
    }

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(mpi_errno,
				     MPIR_ERR_RECOVERABLE,
				     FCNAME,
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**mpi_type_hindexed",
				     "**mpi_type_hindexed %d %p %p %D %p",
				     count,
				     blocklens,
				     indices,
				     old_type,
				     newtype);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
