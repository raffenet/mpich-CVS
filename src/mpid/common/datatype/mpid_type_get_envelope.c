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

/*@
  MPID_Type_get_envelope - get envelope information from datatype

  Input Parameters:
. datatype - MPI datatype

  Output Parameters:
+ num_integers - number of integers used to create datatype
. num_addresses - number of MPI_Aints used to create datatype
. num_datatypes - number of MPI_Datatypes used to create datatype
- combiner - function type used to create datatype
@*/

int MPID_Type_get_envelope(MPI_Datatype datatype,
			   int *num_integers,
			   int *num_addresses,
			   int *num_datatypes,
			   int *combiner)
{
    MPID_Datatype *dtp;
    struct MPID_Dataloop *dlp;
    int dtype_combiner;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
	*num_integers  = 0;
	*num_addresses = 0;
	*num_datatypes = 0;
	*combiner = MPI_COMBINER_NAMED;
	return MPI_SUCCESS;
    }

    MPID_Datatype_get_ptr(datatype, dtp);
    dtype_combiner = dtp->combiner;
    dlp = dtp->loopinfo;

    /* Note: MPI specs indicate that I shouldn't use a switch. */
    if (dtype_combiner == MPI_COMBINER_CONTIGUOUS) {
	*num_integers  = 1; /* count */
	*num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_VECTOR) {
	*num_integers  = 3; /* count, blocklength, stride */
	*num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_HVECTOR) {
	*num_integers  = 2; /* count, blocklength */
	*num_addresses = 1; /* stride */
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_INDEXED) {
	*num_integers  = 1 + 2 * dlp->loop_params.i_t.count; /* count,
                                                              * count blklens
							      * count disps
							      */
        *num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_HINDEXED) {
	*num_integers  = 1 + dlp->loop_params.i_t.count; /* count, 
                                                          * count blklens
							  */
	*num_addresses = dlp->loop_params.i_t.count; /* count disps */
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_STRUCT) {
	*num_integers  = 1 + dlp->loop_params.s_t.count; /* count,
							  * count blklens
							  */
	*num_addresses = dlp->loop_params.s_t.count; /* count disps */
	*num_datatypes = dlp->loop_params.s_t.count; /* count types */
    }
    else if (dtype_combiner == MPI_COMBINER_DUP) {
	*num_integers  = 0;
	*num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_HVECTOR_INTEGER) {
	*num_integers  = 3; /* count, blocklength, stride */
	*num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_HINDEXED_INTEGER) {
	*num_integers  = 1 + 2 * dlp->loop_params.i_t.count; /* count,
                                                              * count blklens
							      * count disps
							      */
        *num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_INDEXED_BLOCK) {
	*num_integers  = 2 + dlp->loop_params.bi_t.count; /* count, blklen,
			 				   * count disps
				 			   */
	*num_addresses = 0;
	*num_datatypes = 1;
    }
    else if (dtype_combiner == MPI_COMBINER_STRUCT_INTEGER) {
	*num_integers  = 1 + 2 * dlp->loop_params.s_t.count; /* count,
							      * count blklens,
							      * count disps
							      */
	*num_addresses = 0;
	*num_datatypes = dlp->loop_params.s_t.count; /* count types */
    }
    else if (dtype_combiner == MPI_COMBINER_SUBARRAY) {
	assert(0);
    }
    else if (dtype_combiner == MPI_COMBINER_DARRAY) {
	assert(0);
    }
    else if (dtype_combiner == MPI_COMBINER_F90_REAL) {
	*num_integers  = 2; /* p and r */
	*num_addresses = 0;
	*num_datatypes = 0;
    }
    else if (dtype_combiner == MPI_COMBINER_F90_COMPLEX) {
	*num_integers  = 2; /* p and r */
	*num_addresses = 0;
	*num_datatypes = 0;
    }
    else if (dtype_combiner == MPI_COMBINER_F90_INTEGER) {
	*num_integers  = 1; /* r */
	*num_addresses = 0;
	*num_datatypes = 0;
    }
    else if (dtype_combiner == MPI_COMBINER_RESIZED) {
	*num_integers  = 0;
	*num_addresses = 2; /* lb, extent */
	*num_datatypes = 1;
    }
    else {
	/* invalid combiner value */
	int mpi_errno;

	mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**dtype", 0);
	return mpi_errno;
    }
    
    *combiner = dtype_combiner;
    return MPI_SUCCESS;
}
