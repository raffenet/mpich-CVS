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
   MPI_Graph_map - graph_map

   Arguments:
+  MPI_Comm comm_old - communicator
.  int nnodes - nnodes
.  int *index - index
.  int *edges - edges
-  int *newrank - newrank

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Graph_map(MPI_Comm comm_old, int nnodes, int *index, int *edges, int *newrank)
{
    static const char FCNAME[] = "MPI_Graph_map";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GRAPH_MAP);
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
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_MAP);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GRAPH_MAP);
    return MPI_SUCCESS;
}
