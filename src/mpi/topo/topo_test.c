/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Topo_test */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Topo_test = PMPI_Topo_test
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Topo_test  MPI_Topo_test
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Topo_test as PMPI_Topo_test
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Topo_test PMPI_Topo_test

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Topo_test

/*@
   MPI_Topo_test - topology test

   Arguments:
+  MPI_Comm comm - communicator
-  int *topo_type - topology type of communicator 'comm' (choice).


   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Topo_test(MPI_Comm comm, int *topo_type)
{
    static const char FCNAME[] = "MPI_Topo_test";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *topo_ptr;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TOPO_TEST);
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
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TOPO_TEST);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    topo_ptr = MPIR_Topology_get( comm_ptr );
    if (topo_ptr)
	*topo_type = (int)(topo_ptr->kind);
    else 
	*topo_type = MPI_UNDEFINED;
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TOPO_TEST);
    return MPI_SUCCESS;
}
