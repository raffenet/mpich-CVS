/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack_external_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack_external_size = PMPI_Pack_external_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack_external_size  MPI_Pack_external_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack_external_size as PMPI_Pack_external_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Pack_external_size
#define MPI_Pack_external_size PMPI_Pack_external_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack_external_size

/*@
  MPI_Pack_external_size - Returns the upper bound on the amount of
  space needed to pack a message using MPI_Pack_external.

   Input Parameters:
+ datarep - data representation (string)  
. incount - number of input data items (integer)  
- datatype - datatype of each input data item (handle)  

   Output Parameters:
. size - output buffer size, in bytes (integer)  

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Pack_external_size(char *datarep,
			   int incount,
			   MPI_Datatype datatype,
			   MPI_Aint *size)
{
    static const char FCNAME[] = "MPI_Pack_external_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK_EXTERNAL_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK_EXTERNAL_SIZE);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(incount,mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Datatype_get_ptr(datatype, datatype_ptr);
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    
    *size = incount * MPID_Datatype_size_external32(datatype);

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK_EXTERNAL_SIZE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_pack_external_size",
	    "**mpi_pack_external_size %s %d %D %p", datarep, incount, datatype, size);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
