/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_shift */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_shift = PMPI_Cart_shift
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_shift  MPI_Cart_shift
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_shift as PMPI_Cart_shift
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Cart_shift PMPI_Cart_shift

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_shift

/*@
MPI_Cart_shift - Returns the shifted source and destination ranks, given a 
                 shift direction and amount

Input Parameters:
+ comm - communicator with cartesian structure (handle) 
. direction - coordinate dimension of shift (integer) 
- disp - displacement (> 0: upwards shift, < 0: downwards shift) (integer) 

Output Parameters:
+ rank_source - rank of source process (integer) 
- rank_dest - rank of destination process (integer) 

Notes:
The 'direction' argument is in the range '[0,n-1]' for an n-dimensional 
Cartesian mesh.

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Cart_shift(MPI_Comm comm, int direction, int displ, int *source, 
		   int *dest)
{
    static const char FCNAME[] = "MPI_Cart_shift";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    int i, n, *vals;
    int pos[MAX_CART_DIM];
    int rank;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_SHIFT);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    MPIR_ERRTEST_ARGNULL( source, "source", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( dest, "dest", mpi_errno );
	    MPIR_ERRTEST_ARGNEG( direction, "direction", mpi_errno );

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_SHIFT);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    cart_ptr = MPIR_Topology_get( comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (!cart_ptr || cart_ptr->kind != MPI_CART) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_TOPOLOGY, 
						  "**notcarttopo", 0 );
	    }
	    else if (direction >= cart_ptr->topo.cart.ndims) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
					  "**dimsmany", "**dimsmany %d %d",
						  cart_ptr->topo.cart.ndims,
						  direction);
	    }
	    if (mpi_errno) {
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_SHIFT);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Check for the dase of a 0 displacement */
    rank = comm_ptr->rank;
    if (displ == 0) {
	*source = *dest = rank;
    }
    else {
	/* To support advanced implementations that support MPI_Cart_create,
	   we compute the new position and call PMPI_Cart_rank to get the
	   source and destination.  We could bypass that step if we know that
	   the mapping is trivial.  Copy the current position. */
	for (i=0; i<cart_ptr->topo.cart.ndims; i++) {
	    pos[i] = cart_ptr->topo.cart.position[i];
	}
	/* We must return MPI_PROC_NULL if shifted over the edge of a 
	   non-periodic mesh */
	pos[direction] += displ;
	if (!cart_ptr->topo.cart.periodic[direction] &&
	    (pos[direction] >= cart_ptr->topo.cart.dims[direction] ||
	     pos[direction] < 0)) {
	    *dest = MPI_PROC_NULL;
	}
	else {
	    (void) PMPI_Cart_rank( comm, pos, dest );
	}
	pos[direction] = cart_ptr->topo.cart.position[i] - displ;
	if (!cart_ptr->topo.cart.periodic[direction] &&
	    (pos[direction] >= cart_ptr->topo.cart.dims[direction] ||
	     pos[direction] < 0)) {
	    *dest = MPI_PROC_NULL;
	}
	else {
	    (void) PMPI_Cart_rank( comm, pos, source );
	}
    }

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_SHIFT);
    return MPI_SUCCESS;
}
