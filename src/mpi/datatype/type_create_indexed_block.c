/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_indexed_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_indexed_block = PMPI_Type_create_indexed_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_indexed_block  MPI_Type_create_indexed_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_indexed_block as PMPI_Type_create_indexed_block
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_indexed_block PMPI_Type_create_indexed_block

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_indexed_block

/*@
   MPI_Type_create_indexed_block - Create an indexed
     datatype with constant-sized blocks

   Input Parameters:
+ count - length of array of displacements (integer) 
. blocklength - size of block (integer) 
. array_of_displacements - array of displacements (array of integer) 
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
int MPI_Type_create_indexed_block(int count,
				  int blocklength,
				  int array_of_displacements[],
				  MPI_Datatype oldtype,
				  MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_indexed_block";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp;
    int i, *ints;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
    /* Get handles to MPI objects. */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;

            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNEG(blocklength, "blocklen", mpi_errno);
	    if (count > 0) {
		MPIR_ERRTEST_ARGNULL(array_of_displacements,
				     "indices",
				     mpi_errno);
	    }
	    MPIR_ERRTEST_DATATYPE_NULL(oldtype, "datatype", mpi_errno);
	    
	    if (HANDLE_GET_KIND(oldtype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(oldtype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    }

            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_blockindexed(count,
				       blocklength,
				       array_of_displacements,
				       0, /* dispinbytes */
				       oldtype,
				       newtype);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
	goto fn_fail;
    /* --END ERROR HANDLING-- */

    ints = (int *) MPIU_Malloc((count + 2) * sizeof(int));
    if (ints == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	goto fn_fail;
    }

    ints[0] = count;
    ints[1] = blocklength;

    for (i=0; i < count; i++)
    {
	ints[i+2] = array_of_displacements[i];
    }

    MPID_Datatype_get_ptr(*newtype, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
				           MPI_COMBINER_INDEXED_BLOCK,
				           count + 2, /* ints */
				           0, /* aints */
				           1, /* types */
				           ints,
				           NULL,
				           &oldtype);
    MPIU_Free(ints);

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**mpi_type_create_indexed_block",
				     "**mpi_type_create_indexed_block %d %d %p %D %p",
				     count, blocklength, array_of_displacements,
				     oldtype, newtype);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
