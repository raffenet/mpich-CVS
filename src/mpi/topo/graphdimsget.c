/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graphdims_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graphdims_get = PMPI_Graphdims_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graphdims_get  MPI_Graphdims_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graphdims_get as PMPI_Graphdims_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Graphdims_get PMPI_Graphdims_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graphdims_get

/*@
   MPI_Graphdims_get - graphdims_get

   Arguments:
+  MPI_Comm comm - communicator
.  int *nnodes - nnodes
-  int *nedges - nedges

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
    static const char FCNAME[] = "MPI_Graphdims_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *topo_ptr;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPHDIMS_GET);
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
	    MPIR_ERRTEST_ARGNULL(nnodes, "nnodes", mpi_errno );
	    MPIR_ERRTEST_ARGNULL(nedges, "nedges", mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPHDIMS_GET);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    topo_ptr = MPIR_Topology_get( comm_ptr );

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (!topo_ptr || topo_ptr->kind != MPI_GRAPH) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_TOPOLOGY, 
						  "**notgraphtopo", 0 );
	    }
	    if (mpi_errno) {
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPHDIMS_GET);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Set nnodes */
    *nnodes = topo_ptr->topo.graph.nnodes;

    /* Set nedges */
    *nedges = topo_ptr->topo.graph.nedges;

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPHDIMS_GET);
    return MPI_SUCCESS;
}
