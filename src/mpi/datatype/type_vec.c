/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "dataloop.h"

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
. newtype - new datatype (handle) 

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_vector(int count, int blocklength, int stride, 
		    MPI_Datatype old_type, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_vector";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *old_ptr = NULL, *new_ptr;

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
	    /* If old_ptr is not value, it will be reset to null */
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

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    new_ptr = MPID_Datatype_create( );
    if (!new_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, 
					  "**nomem" );
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }

    new_ptr->ref_count = 1;
    new_ptr->is_contig = 0;
    new_ptr->is_perm   = 0;
    new_ptr->opt_loopinfo = (struct MPID_Dataloop_st *)MPIU_Calloc( 1, 
				       sizeof(struct MPID_Dataloop_st) );
    new_ptr->loopsize = 1;
    new_ptr->combiner = MPI_COMBINER_VECTOR;
    new_ptr->loopinfo = new_ptr->opt_loopinfo;
    new_ptr->is_permanent = 0;
    new_ptr->is_committed = 0;
    new_ptr->attributes.next = 0;
    new_ptr->cache_id = 0;
    new_ptr->name[0] = 0;
    if (HANDLE_GET_KIND(old_type) == HANDLE_KIND_BUILTIN) {
	/* Avoid using an explicit object for the predefined types */
	int oldsize = (old_type & 0x000000ff);
	new_ptr->size = oldsize * count * blocklength;
	new_ptr->extent = ((count - 1) * stride + blocklength) * oldsize;
	new_ptr->has_mpi1_ub = 0;
	new_ptr->has_mpi1_lb = 0;
	new_ptr->loopinfo_depth = 1;
	new_ptr->true_lb = 0;
	new_ptr->alignsize = oldsize;
	new_ptr->n_elements = count * blocklength;
	new_ptr->element_size = oldsize;
	new_ptr->opt_loopinfo->kind = MPID_VECTOR | DATALOOP_FINAL_MASK | 
	    (oldsize << DATALOOP_ELMSIZE_SHIFT);
	new_ptr->opt_loopinfo->loop_params.v_t.count = count;
	new_ptr->opt_loopinfo->loop_params.v_t.blocksize = blocklength;
	new_ptr->opt_loopinfo->loop_params.v_t.stride = stride;
	new_ptr->opt_loopinfo->loop_params.v_t.dataloop = 0;
	new_ptr->opt_loopinfo->extent = new_ptr->extent;
	new_ptr->opt_loopinfo->id = new_ptr->id;
    }
    else {
	new_ptr->size = old_ptr->size * count * blocklength;
	/* This computation of extent is not correct */
	new_ptr->extent = ((count - 1) * stride + blocklength) * old_ptr->extent;
	new_ptr->has_mpi1_ub = old_ptr->has_mpi1_ub;
	new_ptr->has_mpi1_lb = old_ptr->has_mpi1_lb;
	new_ptr->loopinfo_depth = old_ptr->loopinfo_depth + 1;
	new_ptr->true_lb = old_ptr->true_lb;
	new_ptr->alignsize = old_ptr->alignsize;
	new_ptr->n_elements = old_ptr->n_elements * count * blocklength;
	new_ptr->element_size = old_ptr->element_size;
    }

    *newtype = new_ptr->id;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_VECTOR);
    return MPI_SUCCESS;
}
