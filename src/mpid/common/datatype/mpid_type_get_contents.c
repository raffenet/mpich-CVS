/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>
#include <mpiimpl.h>
#include <mpid_datatype.h>
#include <mpid_dataloop.h>
#include <assert.h>

static int create_error_to_return(void);

/*@
  MPID_Type_get_contents - get content information from datatype

  Input Parameters:
+ datatype - MPI datatype
. max_integers - size of array_of_integers
. max_addresses - size of array_of_addresses
- max_datatypes - size of array_of_datatypes

  Output Parameters:
+ array_of_integers - integers used in creating type
. array_of_addresses - MPI_Aints used in creating type
- array_of_datatypes - MPI_Datatypes used in creating type

@*/
/*
 * TODO: 
 * - Come back and merge common combiners together once it is clear that
 *   they are in fact common.
 * - Combine for loops in cases where appropriate, or use block memory copies,
 *   if we think that would make things faster.
 * - Fix stride and related values on all but vector type; leaving these alone
 *   until we figure out exactly what we want to do with the loopinfo vs.
 *   opt_loopinfo.
 */
int MPID_Type_get_contents(MPI_Datatype datatype, 
			   int max_integers, 
			   int max_addresses, 
			   int max_datatypes, 
			   int array_of_integers[], 
			   MPI_Aint array_of_addresses[], 
			   MPI_Datatype array_of_datatypes[])
{
    MPID_Datatype *dtp;
    struct MPID_Dataloop *dlp;
    int dtype_combiner, count, i;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
	/* this is erroneous according to the standard */
	return create_error_to_return();
    }

    MPID_Datatype_get_ptr(datatype, dtp);
    dtype_combiner = dtp->combiner;
    dlp = dtp->loopinfo;

    if (dtype_combiner == MPI_COMBINER_CONTIGUOUS) {
	if (max_integers >= 1 && max_datatypes >= 1) {
	    array_of_integers[0]  = dlp->loop_params.c_t.count;
	    array_of_datatypes[0] = dlp->loop_params.c_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_VECTOR) {
	if (max_integers >= 3 && max_datatypes >= 1) {
	    if (dlp->kind & DLOOP_FINAL_MASK) {
		array_of_datatypes[0] = dlp->loop_params.v_t.u.handle;
	    }
	    else {
		array_of_datatypes[0] = dlp->loop_params.v_t.u.dataloop->handle;
	    }
	    array_of_integers[0]  = dlp->loop_params.v_t.count;
	    array_of_integers[1]  = dlp->loop_params.v_t.blocksize;
	    array_of_integers[2]  = dlp->loop_params.v_t.stride / dlp->el_extent;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_HVECTOR) {
	if (max_integers >= 2 && max_addresses >= 1 && max_datatypes >= 1) {
	    array_of_integers[0]  = dlp->loop_params.v_t.count;
	    array_of_integers[1]  = dlp->loop_params.v_t.blocksize;
	    array_of_addresses[0] = dlp->loop_params.v_t.stride;
	    if (dlp->kind & DLOOP_FINAL_MASK)
		array_of_datatypes[0] = dlp->loop_params.v_t.u.handle;
	    else
		array_of_datatypes[0] = dlp->loop_params.v_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_INDEXED) {
	if (max_integers >= 1 + 2 * dlp->loop_params.i_t.count &&
	    max_datatypes >= 1)
	{
	    count = dlp->loop_params.i_t.count;
	    array_of_integers[0] = count;
	    for (i=0; i < count; i++) {
		array_of_integers[i+1] = dlp->loop_params.i_t.blocksize_array[i];
	    }
	    for (i=0; i < count; i++) {
		/* divide by element extent to convert back from bytes */
		/* TODO: padding/alignment issues? */
		array_of_integers[i+count+1] = dlp->loop_params.i_t.offset_array[i] / dlp->el_extent;
	    }
	    if (dlp->kind & DLOOP_FINAL_MASK)
		array_of_datatypes[0] = dlp->loop_params.i_t.u.handle;
	    else
		array_of_datatypes[0] = dlp->loop_params.i_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_HINDEXED) {
	if (max_integers >= 1 + dlp->loop_params.i_t.count &&
	    max_addresses >= dlp->loop_params.i_t.count &&
	    max_datatypes >= 1)
	{
	    count = dlp->loop_params.i_t.count;
	    array_of_integers[0] = count;
	    for (i=0; i < count; i++) {
		array_of_integers[i+1] = dlp->loop_params.i_t.blocksize_array[i];
	    }
	    for (i=0; i < count; i++) {
		array_of_addresses[i] = dlp->loop_params.i_t.offset_array[i];
	    }
	    if (dlp->kind & DLOOP_FINAL_MASK)
		array_of_datatypes[0] = dlp->loop_params.i_t.u.handle;
	    else
		array_of_datatypes[0] = dlp->loop_params.i_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_STRUCT) {
	if (max_integers >= 1 + dlp->loop_params.s_t.count &&
	    max_addresses >= dlp->loop_params.s_t.count &&
	    max_datatypes >= dlp->loop_params.s_t.count)
	{
	    count = dlp->loop_params.s_t.count;
	    array_of_integers[0] = count;
	    for (i=0; i < count; i++) {
		array_of_integers[i+1] = dlp->loop_params.s_t.blocksize_array[i];
	    }
	    for (i=0; i < count; i++) {
		array_of_addresses[i] = dlp->loop_params.s_t.offset_array[i];
	    }
	    for (i=0; i < count; i++) {
		array_of_datatypes[i] = dlp->loop_params.s_t.dataloop_array[i]->handle;
	    }
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_DUP) {
	if (max_datatypes >= 1) {
	    assert(0);
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_HVECTOR_INTEGER) {
	if (max_integers >= 3 && max_datatypes >= 1) {
	    array_of_integers[0]  = dlp->loop_params.v_t.count;
	    array_of_integers[0]  = dlp->loop_params.v_t.blocksize;
	    array_of_integers[0]  = dlp->loop_params.v_t.stride;
	    if (dlp->kind & DLOOP_FINAL_MASK)
		array_of_datatypes[0] = dlp->loop_params.v_t.u.handle;
	    else
		array_of_datatypes[0] = dlp->loop_params.v_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_HINDEXED_INTEGER) {
	if (max_integers >= 1 + 2 * dlp->loop_params.i_t.count &&
	    max_datatypes >= 1)
	{
	    count = dlp->loop_params.i_t.count;
	    array_of_integers[0] = count;
	    for (i=0; i < count; i++) {
		array_of_integers[i+1] = dlp->loop_params.i_t.blocksize_array[i];
	    }
	    for (i=0; i < count; i++) {
		array_of_integers[i+count+1] = dlp->loop_params.i_t.offset_array[i];
	    }
	    if (dlp->kind & DLOOP_FINAL_MASK)
		array_of_datatypes[0] = dlp->loop_params.i_t.u.handle;
	    else
		array_of_datatypes[0] = dlp->loop_params.i_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_INDEXED_BLOCK) {
	if (max_integers >= 2 + dlp->loop_params.bi_t.count &&
	    max_datatypes >= 1)
	{
	    count = dlp->loop_params.bi_t.count;
	    array_of_integers[0] = count;
	    array_of_integers[1] = dlp->loop_params.bi_t.blocksize;
	    for (i=0; i < count; i++) {
		array_of_integers[i+2] = dlp->loop_params.bi_t.offset_array[i];
	    }
	    if (dlp->kind & DLOOP_FINAL_MASK)
		array_of_datatypes[0] = dlp->loop_params.bi_t.u.handle;
	    else
		array_of_datatypes[0] = dlp->loop_params.bi_t.u.dataloop->handle;
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_STRUCT_INTEGER) {
	if (max_integers >= 1 + 2 * dlp->loop_params.s_t.count && 
	    max_datatypes >= dlp->loop_params.s_t.count)
	{
	    count = dlp->loop_params.s_t.count;
	    array_of_integers[0] = count;
	    for (i=0; i < count; i++) {
		array_of_integers[i+1] = dlp->loop_params.s_t.blocksize_array[i];
	    }
	    for (i=0; i < count; i++) {
		array_of_integers[i+count+1] = dlp->loop_params.s_t.offset_array[i];
	    }
	    for (i=0; i < count; i++) {
		array_of_datatypes[i] = dlp->loop_params.s_t.dataloop_array[i]->handle;
	    }
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_SUBARRAY) {
	assert(0);
    }
    else if (dtype_combiner == MPI_COMBINER_DARRAY) {
	assert(0);
    }
    else if (dtype_combiner == MPI_COMBINER_F90_REAL) {
	if (max_integers >= 2) {
	    assert(0);
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_F90_COMPLEX) {
	if (max_integers >= 2) {
	    assert(0);
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_F90_INTEGER) {
	if (max_integers >= 1) {
	    assert(0);
	}
	else return create_error_to_return();
    }
    else if (dtype_combiner == MPI_COMBINER_RESIZED) {
	if (max_addresses >= 2 && max_datatypes >= 1) {
	    assert(0);
	}
	else return create_error_to_return();
    }
    else {
	/* invalid combiner value */
	return create_error_to_return();
    }

    return MPI_SUCCESS;
}

/* create_error_to_return() - create an error code and return the value
 *
 * Basically there are a million points in this function where I need to do
 * this, so I created a static function to do it instead.
 */
static int create_error_to_return(void)
{
    int mpi_errno;

    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**dtype", 0);
    return mpi_errno;
}
