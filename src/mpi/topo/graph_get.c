/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_get = PMPI_Graph_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_get  MPI_Graph_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_get as PMPI_Graph_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Graph_get PMPI_Graph_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_get

/*@
   MPI_Graph_get - graph_get

   Arguments:
+  MPI_Comm comm - communicator
.  int maxindex - max index
.  int maxedges - max edges
.  int *index - index
-  int *edges - edges

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int *index, int *edges)
{
    static const char FCNAME[] = "MPI_Graph_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Topology *topo_ptr;
    int i, n, *vals;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_GET);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_GET);
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
	    
	    MPIR_ERRTEST_ARGNULL( edges, "edges", mpi_errno );
	    MPIR_ERRTEST_ARGNULL( index,  "index", mpi_errno );

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_GET);
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
	    else if (topo_ptr->topo.graph.nnodes > maxindex) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
					  "**argrange", "**argrange %s %d %d",
					  "maxindex", maxindex, 
					  topo_ptr->topo.graph.nnodes );
	    }
	    else if (topo_ptr->topo.graph.nedges > maxedges) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
					  "**argrange", "**argrange %s %d %d",
					  "maxedges", maxedges, 
					  topo_ptr->topo.graph.nedges );
	    }
	    if (mpi_errno) {
		MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_GET);
		return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Get index */
    n = topo_ptr->topo.graph.nnodes;
    vals = topo_ptr->topo.graph.index;
    for ( i=0; i<n; i++ )
	*index++ = *vals++;
	    
    /* Get edges */
    n = topo_ptr->topo.graph.nedges;
    vals = topo_ptr->topo.graph.edges;
    for ( i=0; i<n; i++ )
	*edges++ = *vals++;

    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_GET);
    return MPI_SUCCESS;
}
