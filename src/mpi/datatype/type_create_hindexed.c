/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_hindexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_hindexed = PMPI_Type_create_hindexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_hindexed  MPI_Type_create_hindexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_hindexed as PMPI_Type_create_hindexed
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_hindexed PMPI_Type_create_hindexed

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_hindexed

/*@
   MPI_Type_create_hindexed - Create a datatype for an indexed datatype with 
   displacements in bytes

   Input Parameters:
+ count - number of blocks --- also number of entries in 
  displacements and blocklengths (integer) 
. blocklengths - number of elements in each block (array of nonnegative integers) 
. displacements - byte displacement of each block (array of integer) 
- oldtype - old datatype (handle) 

   Output Parameter:
. newtype - new datatype (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_hindexed(int count,
			     int blocklengths[],
			     MPI_Aint displacements[],
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_hindexed";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp;
    int i, *ints;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int i;
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(blocklengths, "blocklens", mpi_errno);
		MPIR_ERRTEST_ARGNULL(displacements, "indices", mpi_errno);
	    }
	    MPIR_ERRTEST_DATATYPE_NULL(oldtype, "datatype", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
		    MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
		}
		for (i=0; i < count; i++) {
		    MPIR_ERRTEST_ARGNEG(blocklengths[i], "blocklen", mpi_errno);
		}
	    }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_indexed(count,
				  blocklengths,
				  displacements,
				  1, /* displacements in bytes */
				  oldtype,
				  newtype);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
	goto fn_fail;
    /* --END ERROR HANDLING-- */

    ints = (int *) MPIU_Malloc((count + 1) * sizeof(int));
    if (ints == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	goto fn_fail;
    }

    ints[0] = count;

    for (i=0; i < count; i++)
    {
	ints[i+1] = blocklengths[i];
    }
    MPID_Datatype_get_ptr(*newtype, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_HINDEXED,
				           count+1, /* ints (count, blocklengths) */
				           count, /* aints (displacements) */
				           1, /* types */
				           ints,
				           displacements,
				           &oldtype);
    MPIU_Free(ints);

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno,
				     MPIR_ERR_RECOVERABLE,
				     FCNAME,
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**mpi_type_create_hindexed",
				     "**mpi_type_create_hindexed %d %p %p %D %p",
				     count,
				     blocklengths,
				     displacements,
				     oldtype,
				     newtype);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_HINDEXED);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
