/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_vector */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_vector = PMPI_Type_vector
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_vector  MPI_Type_vector
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_vector as PMPI_Type_vector
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_vector PMPI_Type_vector

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_vector

/*@
    MPI_Type_vector - Creates a vector (strided) datatype

Input Parameters:
+ count - number of blocks (nonnegative integer) 
. blocklength - number of elements in each block 
(nonnegative integer) 
. stride - number of elements between start of each block (integer) 
- oldtype - old datatype (handle) 

Output Parameter:
. newtype_p - new datatype (handle) 

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_vector(int count,
		    int blocklength,
		    int stride, 
		    MPI_Datatype old_type,
		    MPI_Datatype *newtype_p)
{
    static const char FCNAME[] = "MPI_Type_vector";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_VECTOR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_VECTOR);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *old_ptr = NULL;

	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_ARGNEG(blocklength, "blocklen", mpi_errno);
	    MPIR_ERRTEST_DATATYPE_NULL(old_type, "datatype", mpi_errno);
	    if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(old_type, old_ptr);
		MPID_Datatype_valid_ptr(old_ptr, mpi_errno);
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
                return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_vector(count,
				 blocklength,
				 (MPI_Aint) stride,
				 0, /* stride not in bytes, but in terms of type extent */
				 old_type,
				 newtype_p);

    if (mpi_errno == MPI_SUCCESS) {
	MPID_Datatype *new_dtp;
	int ints[3];

	ints[0] = count;
	ints[1] = blocklength;
	ints[2] = stride;
	MPID_Datatype_get_ptr(*newtype_p, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       MPI_COMBINER_VECTOR,
					       3, /* ints (count, blocklength, stride) */
					       0, /* aints */
					       1, /* types */
					       ints,
					       NULL,
					       &old_type);
    }

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
        return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME,
				     __LINE__, MPI_ERR_OTHER,
				     "**mpi_type_vector",
				     "**mpi_type_vector %d %d %d %D %p", count,
				     blocklength, stride, old_type, newtype_p);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}







