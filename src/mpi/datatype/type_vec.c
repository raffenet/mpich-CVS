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
int MPI_Type_vector(int count, int blocklength, int stride, 
		    MPI_Datatype old_type, MPI_Datatype *newtype_p)
{
    static const char FCNAME[] = "MPI_Type_vector";
    int ret;
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *old_ptr = NULL;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_VECTOR);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_VECTOR);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( old_type, old_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate old_ptr */
	    if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_valid_ptr( old_ptr, mpi_errno );
	    }
	    /* If old_ptr is not valid, it will be reset to null */
	    /* Validate other arguments */
	    if (count < 0) 
		mpi_errno = MPIR_Err_create_code( MPI_ERR_COUNT, "**countneg",
						  "**countneg %d", count );
	    if (blocklength < 0) 
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, "**argneg",
						  "**argneg %s %d", 
						  "blocklength", blocklength );
	    /* MPICH 1 code also checked for old type equal to MPI_UB or LB.
	       We may want to check on length 0 datatypes */

	    /* Note: if there are multiple errors, only the last one detected
	     * will be reported.
	     */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    ret = MPID_Type_vector(count, blocklength, stride, old_type, newtype_p);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
    if (ret == MPI_SUCCESS) return MPI_SUCCESS;
    else return MPIR_Err_return_comm(0, FCNAME, ret);

}







