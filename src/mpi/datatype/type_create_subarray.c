/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if 0

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_subarray */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_subarray = PMPI_Type_create_subarray
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_subarray  MPI_Type_create_subarray
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_subarray as PMPI_Type_create_subarray
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_subarray PMPI_Type_create_subarray

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_subarray

/*@
   MPI_Type_create_subarray - create datatype subarray

   Input Parameters:
+ ndims - number of array dimensions (positive integer) 
. array_of_sizes - number of elements of type oldtype in each dimension of the
  full array (array of positive integers) 
. array_of_subsizes - number of elements of type oldtype in each dimension of
  the subarray (array of positive integers) 
. array_of_starts - starting coordinates of the subarray in each dimension
  (array of nonnegative integers) 
. order - array storage order flag (state) 
- oldtype - array element datatype (handle) 

   Output Parameter:
. newtype new datatype (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_create_subarray(int ndims,
			     int array_of_sizes[],
			     int array_of_subsizes[],
			     int array_of_starts[],
			     int order,
			     MPI_Datatype oldtype,
			     MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_subarray";
    int mpi_errno = MPI_SUCCESS, i;

    /* these variables are from the original version in ROMIO */
    MPI_Aint size, extent, disps[3];
    int blklens[3];
    MPI_Datatype tmp1, tmp2, types[3];

    /* for saving contents */
    int *ints;
    MPID_Datatype *new_dtp;

    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

	    /* Check parameters */
	    MPIR_ERRTEST_COUNT(ndims, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_sizes, "array_of_sizes", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_subsizes, "array_of_subsizes", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_starts, "array_of_starts", mpi_errno);
	    for (i=0; mpi_errno == MPI_SUCCESS && i < ndims; i++) {
		MPIR_ERRTEST_ARGNONPOS(array_of_sizes[i], "size", mpi_errno);
		MPIR_ERRTEST_ARGNONPOS(array_of_subsizes[i], "subsize", mpi_errno);
		MPIR_ERRTEST_ARGNEG(array_of_starts[i], "start", mpi_errno);
		if (array_of_subsizes[i] > array_of_sizes[i]) {
		    /* TODO: THIS IS AN ERROR */
		}
		if (array_of_starts[i] > (array_of_sizes[i] - array_of_subsizes[i]))
		{
		    /* TODO: THIS IS AN ERROR */
		}
		if (order != MPI_ORDER_FORTRAN && order != MPI_ORDER_C) {
		    /* TODO: THIS IS AN ERROR */
		}
	    }

#if 0
	    /* TODO: INTEGRATE THIS CHECK AS WELL */
	    MPI_Type_extent(oldtype, &extent);

	    /* check if MPI_Aint is large enough for size of global array. 
	       if not, complain. */

	    size_with_aint = extent;
	    for (i=0; i<ndims; i++) size_with_aint *= array_of_sizes[i];
	    size_with_offset = extent;
	    for (i=0; i<ndims; i++) size_with_offset *= array_of_sizes[i];
	    if (size_with_aint != size_with_offset) {
		FPRINTF(stderr, "MPI_Type_create_subarray: Can't use an array of this size unless the MPI implementation defines a 64-bit MPI_Aint\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
    }
#endif

            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
                return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* TODO: CHECK THE ERROR RETURNS FROM ALL THESE!!! */

    /* TODO: GRAB EXTENT WITH A MACRO OR SOMETHING FASTER */
    MPI_Type_extent(oldtype, &extent);

    if (order == MPI_ORDER_FORTRAN) {
	if (ndims == 1)
	    mpi_errno = MPID_Type_contiguous(array_of_subsizes[0],
					     oldtype,
					     &tmp1);
	else {
	    mpi_errno = MPID_Type_vector(array_of_subsizes[1],
					 array_of_subsizes[0],
					 array_of_sizes[0],
					 0, /* stride in types */
					 oldtype,
					 &tmp1);
	    
	    size = array_of_sizes[0]*extent;
	    for (i=2; i<ndims; i++) {
		size *= array_of_sizes[i-1];
		mpi_errno = MPID_Type_vector(array_of_subsizes[i],
					     1,
					     size,
					     1, /* stride in bytes */
					     tmp1,
					     &tmp2);
		MPI_Type_free(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	
	disps[1] = array_of_starts[0];
	size = 1;
	for (i=1; i<ndims; i++) {
	    size *= array_of_sizes[i-1];
	    disps[1] += size*array_of_starts[i];
	}  
        /* rest done below for both Fortran and C order */
    }
    else /* MPI_ORDER_C */ {
	/* dimension ndims-1 changes fastest */
	if (ndims == 1)
	    mpi_errno = MPID_Type_contiguous(array_of_subsizes[0],
					     oldtype,
					     &tmp1);
	else {
	    mpi_errno = MPID_Type_vector(array_of_subsizes[ndims-2],
					 array_of_subsizes[ndims-1],
					 array_of_sizes[ndims-1],
					 0, /* stride in types */
					 oldtype,
					 &tmp1);
	    
	    size = array_of_sizes[ndims-1]*extent;
	    for (i=ndims-3; i>=0; i--) {
		size *= array_of_sizes[i+1];
		mpi_errno = MPID_Type_vector(array_of_subsizes[i],
					     1,
					     size,
					     1, /* stride in bytes */
					     tmp1,
					     &tmp2);
		MPI_Type_free(&tmp1);
		tmp1 = tmp2;
	    }
	}
	
	/* add displacement and UB */
	
	disps[1] = array_of_starts[ndims-1];
	size = 1;
	for (i=ndims-2; i>=0; i--) {
	    size *= array_of_sizes[i+1];
	    disps[1] += size*array_of_starts[i];
	}
    }

    disps[1] *= extent;
    
    disps[2] = extent;
    for (i=0; i<ndims; i++) disps[2] *= array_of_sizes[i];
    
    disps[0] = 0;
    blklens[0] = blklens[1] = blklens[2] = 1;
    types[0] = MPI_LB;
    types[1] = tmp1;
    types[2] = MPI_UB;
    
    /* TODO: CAN WE DO THIS MORE SIMPLY? */
    mpi_errno = MPID_Type_struct(3,
				 blklens,
				 disps,
				 types,
				 newtype);

    MPI_Type_free(&tmp1);

    /* at this point we have the new type, and we've cleaned up any
     * intermediate types created in the process.  we just need to save
     * all our contents/envelope information.
     */

    /* Save contents */
    ints = (int *) MPIU_Malloc((3 * ndims + 2) * sizeof(int));
    assert(ints != NULL);

    ints[0] = ndims;
    for (i=0; i < ndims; i++) {
	ints[i + 1] = array_of_sizes[i];
    }
    for(i=0; i < ndims; i++) {
	ints[i + ndims + 1] = array_of_subsizes[i];
    }
    for(i=0; i < ndims; i++) {
	ints[i + 2*ndims + 1] = array_of_starts[i];
    }
    ints[3*ndims + 1] = order;

    MPID_Datatype_get_ptr(*newtype, new_dtp);
    mpi_errno = MPID_Datatype_set_contents(new_dtp,
					   MPI_COMBINER_SUBARRAY,
					   3 * ndims + 2, /* ints */
					   0, /* aints */
					   1, /* types */
					   ints,
					   NULL,
					   &oldtype);

    MPIU_Free(ints);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_SUBARRAY);
    if (mpi_errno == MPI_SUCCESS) return MPI_SUCCESS;
    else return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
}

#endif
