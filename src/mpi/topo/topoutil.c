/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpiimpl.h"
#include "topo.h"

/* Keyval for topology information */
static int MPIR_Topology_keyval = MPI_KEYVAL_INVALID;  

/* Local functions */
static int MPIR_Topology_copy_fn ( MPI_Comm, int, void *, void *, void *, 
				   int * );
static int MPIR_Topology_delete_fn ( MPI_Comm, int, void *, void * );
static void MPIR_Topology_finalize ( void );

/*
  Return a poiner to the topology structure on a communicator.
  Returns null if no topology structure is defined 
*/
MPIR_Topology *MPIR_Topology_get( MPID_Comm *comm_ptr )
{
    MPIR_Topology *topo_ptr;
    int flag;

    if (MPIR_Topology_keyval == MPI_KEYVAL_INVALID) {
	return 0;
    }
    MPIR_Nest_incr();
    (void)PMPI_Comm_get_attr(comm_ptr->handle, MPIR_Topology_keyval,
			     &topo_ptr, &flag );
    MPIR_Nest_decr();
    if (flag) return topo_ptr;
    return 0;
}



