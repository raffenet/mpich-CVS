/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_get = PMPI_Cart_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_get  MPI_Cart_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_get as PMPI_Cart_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Cart_get PMPI_Cart_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_get

/*@
   MPI_Cart_get - cart_get

   Arguments:
+  MPI_Comm comm - communicator
.  int maxdims - maxdims
.  int *dims - dims
.  int *periods - periods
-  int *coords - coords

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, int *periods, int *coords)
{
    static const char FCNAME[] = "MPI_Cart_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *cart_ptr;
    int i, n, *vals;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_GET);
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

	    MPIR_ERRTEST_ARGNULL( dims, "dims", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( periods, "periods", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( coords, "coords", mpi_errno ); 

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_GET);
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
	    else if (cart_ptr->topo.cart.ndims > maxdims) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
					  "**argrange", "**argrange %s %d %d",
					  "maxdims", maxdims, 
					  cart_ptr->topo.cart.ndims );
	    }
	    if (mpi_errno) {
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_GET);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    n = cart_ptr->topo.cart.ndims;
    vals = cart_ptr->topo.cart.dims;
    for ( i=0; i<n; i++ )
	*dims++ = *vals++;
    
    /* Get periods */
    n = cart_ptr->topo.cart.ndims;
    vals = cart_ptr->topo.cart.periodic;
    for ( i=0; i<n; i++ )
	*periods++ = *vals++;
    
    /* Get coords */
    n = cart_ptr->topo.cart.ndims;
    vals = cart_ptr->topo.cart.position;
    for ( i=0; i<n; i++ )
	*coords++ = *vals++;
    
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_GET);
    return MPI_SUCCESS;
}
