/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Graph_map */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Graph_map = PMPI_Graph_map
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Graph_map  MPI_Graph_map
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Graph_map as PMPI_Graph_map
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Graph_map PMPI_Graph_map

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Graph_map

/*@
MPI_Graph_map - Maps process to graph topology information

Input Parameters:
+ comm - input communicator (handle) 
. nnodes - number of graph nodes (integer) 
. index - integer array specifying the graph structure, see 'MPI_GRAPH_CREATE' 
- edges - integer array specifying the graph structure 

Output Parameter:
. newrank - reordered rank of the calling process; 'MPI_UNDEFINED' if the 
calling process does not belong to graph (integer) 
 
.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TOPOLOGY
.N MPI_ERR_COMM
.N MPI_ERR_ARG
@*/
int MPI_Graph_map(MPI_Comm comm_old, int nnodes, int *index, int *edges, int *newrank)
{
    static const char FCNAME[] = "MPI_Graph_map";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GRAPH_MAP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_MAP);
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
	    MPIR_ERRTEST_ARGNULL(index,"index",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(edges,"edges",mpi_errno);
	    MPIR_ERRTEST_ARGNONPOS(nnodes,"nnodes",mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_MAP);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (comm_ptr->local_size < nnodes) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_MAP);
	mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, "**graphnnodes", 0 );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    
    /* This is the trivial version that does not remap any processes.
       FIXME: Add hook to optional device support for process mapping */
    if (comm_ptr->rank < nnodes) {
	*newrank = comm_ptr->rank;
    }
    else {
	*newrank = MPI_UNDEFINED;
    }
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_MAP);
    return MPI_SUCCESS;
}
