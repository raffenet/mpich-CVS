/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_map */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_map = PMPI_Cart_map
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_map  MPI_Cart_map
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_map as PMPI_Cart_map
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Cart_map PMPI_Cart_map

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_map

/*@
MPI_Cart_map - Maps process to Cartesian topology information 

Input Parameters:
+ comm - input communicator (handle) 
. ndims - number of dimensions of Cartesian structure (integer) 
. dims - integer array of size 'ndims' specifying the number of processes in 
  each coordinate direction 
- periods - logical array of size 'ndims' specifying the periodicity 
  specification in each coordinate direction 

Output Parameter:
. newrank - reordered rank of the calling process; 'MPI_UNDEFINED' if 
  calling process does not belong to grid (integer) 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_DIMS
.N MPI_ERR_ARG
@*/
int MPI_Cart_map(MPI_Comm comm_old, int ndims, int *dims, int *periods, 
		 int *newrank)
{
    static const char FCNAME[] = "MPI_Cart_map";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int i, nranks, rank, size;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_CART_MAP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_MAP);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm_old, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    MPIR_ERRTEST_ARGNULL(newrank,"newrank",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(dims,"dims",mpi_errno);
	    if (ndims < 1) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_DIMS,
						  "**dims", "**dims %d", 
						  ndims );
		}
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Determine number of processes needed for topology */
    nranks = dims[0];
    for ( i=1; i<ndims; i++ )
	nranks *= dims[i];
    
    size = comm_ptr->remote_size;
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    
	    /* Test that the communicator is large enough */
	    if (size < nranks) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                         MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_DIMS,
						  "**topotoolarge",
						  "**topotoolarge %d %d",
						  size, nranks );
                goto fn_fail;
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Am I in this range? */
    rank = comm_ptr->rank;
    if ( rank < nranks )
	/* This relies on the ranks *not* being reordered by the current
	   Cartesian routines */
	*newrank = rank;
    else
	*newrank = MPI_UNDEFINED;
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_MAP);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_cart_map", "**mpi_cart_map %C %d %p %p %p", comm_old, ndims, dims, periods, newrank);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_MAP);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
