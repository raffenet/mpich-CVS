/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if 0

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_darray */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_darray = PMPI_Type_create_darray
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_darray  MPI_Type_create_darray
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_darray as PMPI_Type_create_darray
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_darray PMPI_Type_create_darray

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_darray

/*@
   MPI_Type_create_darray - create darray datatype

   Input Parameters:
+ size - size of process group (positive integer) 
. rank - rank in process group (nonnegative integer) 
. ndims - number of array dimensions as well as process grid dimensions (positive integer) 
. array_of_gsizes - number of elements of type oldtype in each dimension of global array (array of positive integers) 
. array_of_distribs - distribution of array in each dimension (array of state) 
. array_of_dargs - distribution argument in each dimension (array of positive integers) 
. array_of_psizes - size of process grid in each dimension (array of positive integers) 
. order - array storage order flag (state) 
- oldtype - old datatype (handle) 

    Output Parameter:
. newtype - new datatype (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_darray(int size, int rank, int ndims, int array_of_gsizes[], int array_of_distribs[], int array_of_dargs[], int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_darray";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_DARRAY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_DARRAY);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_DARRAY);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* FIXME UNIMPLEMENTED */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_DARRAY);
    return MPI_SUCCESS;
}

#endif
