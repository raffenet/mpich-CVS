/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack_size = PMPI_Pack_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack_size  MPI_Pack_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack_size as PMPI_Pack_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Pack_size PMPI_Pack_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack_size

/*@
   MPI_Pack_size - Returns the upper bound on the amount of space needed to
                    pack a message

Input Parameters:
+ incount - count argument to packing call (integer) 
. datatype - datatype argument to packing call (handle) 
- comm - communicator argument to packing call (handle) 

Output Parameter:
. size - upper bound on size of packed message, in bytes (integer) 

Notes:
The MPI standard document describes this in terms of 'MPI_Pack', but it 
applies to both 'MPI_Pack' and 'MPI_Unpack'.  That is, the value 'size' is 
the maximum that is needed by either 'MPI_Pack' or 'MPI_Unpack'.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TYPE
.N MPI_ERR_ARG

@*/
int MPI_Pack_size(int incount,
		  MPI_Datatype datatype,
		  MPI_Comm comm,
		  int *size)
{
    static const char FCNAME[] = "MPI_Pack_size";
    MPID_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    int typesize;
#ifdef HAVE_ERROR_CHECKING
    MPID_Datatype *datatype_ptr = NULL;
#endif
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK_SIZE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK_SIZE);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(incount, mpi_errno);
	    MPIR_ERRTEST_DATATYPE_NULL(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(size, "size", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    MPID_Comm_get_ptr( comm, comm_ptr );
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Datatype_get_size_macro(datatype, typesize);
    *size = incount * typesize;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK_SIZE);
    return MPI_SUCCESS;
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_pack_size", "**mpi_pack_size %d %D %C %p", incount, datatype, comm, size);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK_SIZE);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}










