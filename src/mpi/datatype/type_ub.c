/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_ub */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_ub = PMPI_Type_ub
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_ub  MPI_Type_ub
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_ub as PMPI_Type_ub
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_ub PMPI_Type_ub

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_ub

/*@
    MPI_Type_ub - Returns the upper bound of a datatype

Input Parameters:
. datatype - datatype (handle) 

Output Parameter:
. displacement - displacement of upper bound from origin, 
                             in bytes (integer) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_ub(MPI_Datatype datatype, MPI_Aint *displacement)
{
    static const char FCNAME[] = "MPI_Type_ub";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_UB);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_UB);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_UB);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
	*displacement = MPID_Datatype_get_basic_size(datatype);
    else
	*displacement = datatype_ptr->ub;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_UB);
    return MPI_SUCCESS;
}
