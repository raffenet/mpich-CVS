/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Cart_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Cart_create = PMPI_Cart_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Cart_create  MPI_Cart_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Cart_create as PMPI_Cart_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Cart_create PMPI_Cart_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Cart_create

/*@
   MPI_Cart_create - cart_create

   Arguments:
+  MPI_Comm comm_old - communicator
.  int ndims - ndims
.  int *dims - dims
.  int *periods - periods
.  int reorder - reorder
-  MPI_Comm *comm_cart - communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods, int reorder, MPI_Comm *comm_cart)
{
    static const char FCNAME[] = "MPI_Cart_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_CART_CREATE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm_old, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_CREATE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_CART_CREATE);
    return MPI_SUCCESS;
}
