/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_lb */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_lb = PMPI_Type_lb
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_lb  MPI_Type_lb
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_lb as PMPI_Type_lb
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_lb PMPI_Type_lb

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_lb

/*@
    MPI_Type_lb - Returns the lower-bound of a datatype

Input Parameters:
. datatype - datatype (handle) 

Output Parameter:
. displacement - displacement of lower bound from origin, 
                             in bytes (integer) 

.N Deprecated
The replacement for this routine is 'MPI_Type_Get_extent'.

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_lb(MPI_Datatype datatype, MPI_Aint *displacement)
{
    static const char FCNAME[] = "MPI_Type_lb";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_LB);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_LB);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) goto fn_fail;
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) *displacement = 0;
    else *displacement = datatype_ptr->lb;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_LB);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**mpi_type_lb", "**mpi_type_lb %D %p", 
				     datatype, displacement);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_LB);
    return MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}
