/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_coords */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_coords = PMPI_Cart_coords
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_coords  MPI_Cart_coords
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_coords as PMPI_Cart_coords
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Cart_coords PMPI_Cart_coords

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_coords

/*@
MPI_Cart_coords - Determines process coords in cartesian topology given
                  rank in group

Input Parameters:
+ comm - communicator with cartesian structure (handle) 
. rank - rank of a process within group of 'comm' (integer) 
- maxdims - length of vector 'coords' in the calling program (integer) 

Output Parameter:
. coords - integer array (of size 'ndims') containing the Cartesian 
  coordinates of specified process (integer) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_RANK
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords)
{
    static const char FCNAME[] = "MPI_Cart_coords";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    int i, nnodes;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_COORDS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_COORDS);
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
	    MPIR_ERRTEST_ARGNULL(coords,"coords",mpi_errno);
            if (mpi_errno) goto fn_fail;
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
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TOPOLOGY, 
						  "**notcarttopo", 0 );
	    }
	    else if (cart_ptr->topo.cart.ndims > maxdims) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, 
					  "**dimsmany", "**dimsmany %d %d",
						  cart_ptr->topo.cart.ndims,
						  maxdims );
	    }
	    if (mpi_errno) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Calculate coords */
    nnodes = cart_ptr->topo.cart.nnodes;
    for ( i=0; i < cart_ptr->topo.cart.ndims; i++ ) {
	nnodes    = nnodes / cart_ptr->topo.cart.dims[i];
	coords[i] = rank / nnodes;
	rank      = rank % nnodes;
    }

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_COORDS);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_cart_coords", "**mpi_cart_coords %C %d %d %p", comm, rank, maxdims, coords);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_COORDS);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
