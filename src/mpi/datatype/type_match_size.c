/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_match_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_match_size = PMPI_Type_match_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_match_size  MPI_Type_match_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_match_size as PMPI_Type_match_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_match_size PMPI_Type_match_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_match_size

/*@
   MPI_Type_match_size - Find an MPI datatype matching a specified size

   Input Parameters:
+ typeclass - generic type specifier (integer) 
- size - size, in bytes, of representation (integer) 

   Output Parameter:
. type - datatype with correct type, size (handle) 

Notes:
'typeclass' is one of 'MPI_TYPECLASS_REAL', 'MPI_TYPECLASS_INTEGER' and 
'MPI_TYPECLASS_COMPLEX', corresponding to the desired typeclass. 
The function returns an MPI datatype matching a local variable of type 
'( typeclass, size )'. 


.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
    static const char FCNAME[] = "MPI_Type_match_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_MATCH_SIZE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_MATCH_SIZE);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( *datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_MATCH_SIZE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* FIXME UNIMPLEMENTED */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_MATCH_SIZE);
    return MPI_SUCCESS;
}
