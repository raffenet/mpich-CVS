/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_size = PMPI_Type_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_size  MPI_Type_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_size as PMPI_Type_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_size PMPI_Type_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_size

/*@
    MPI_Type_size - Return the number of bytes occupied by entries
                    in the datatype

Input Parameters:
. datatype - datatype (handle) 

Output Parameter:
. size - datatype size (integer) 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_size(MPI_Datatype datatype, int *size)
{
    static const char FCNAME[] = "MPI_Type_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_SIZE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_DATATYPE(0, datatype, mpi_errno);
            if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_SIZE);
    /* ... body of routine ...  */

    /* If this is a built-in datatype, then get the size out of the handle */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    {
	MPID_Datatype_get_size_macro(datatype, *size);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_SIZE);
	return MPI_SUCCESS;
    }

    /* Convert handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );

    /* Validate objects if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Datatype_get_size_macro(datatype, *size);

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_SIZE);
    return MPI_SUCCESS;
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_type_size", 
				     "**mpi_type_size %D %p", datatype, size);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_SIZE);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
}
