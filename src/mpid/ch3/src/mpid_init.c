/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

MPIDI_Process_t MPIDI_Process;

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int * argc, char *** argp, int requested, int * provided,
	      int * has_args, int * has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
    MPID_Collops * collops;

    MPIDI_dbg_printf(10, FCNAME, "entering");
    
    MPIDI_Process.recv_posted_head = NULL;
    MPIDI_Process.recv_posted_tail = NULL;
    MPIDI_Process.recv_unexpected_head = NULL;
    MPIDI_Process.recv_unexpected_tail = NULL;

    /*
     * Initialize the collective operations operations for the MPI_COMM_WORLD
     * and MPI_COMM_SELF
     */
    collops = MPIU_Calloc(1, sizeof(MPID_Collops));
    assert(collops != NULL);

    collops->ref_count = 2;
    collops->Barrier = MPIDI_Barrier;
    collops->Bcast = NULL;
    collops->Gather = NULL;
    collops->Gatherv = NULL;
    collops->Scatter = NULL;
    collops->Scatterv = NULL;
    collops->Allgather = NULL;
    collops->Allgatherv = NULL;
    collops->Alltoall = NULL;
    collops->Alltoallv = NULL;
    collops->Alltoallw = NULL;
    collops->Reduce = NULL;
    collops->Allreduce = NULL;
    collops->Reduce_scatter = NULL;
    collops->Scan = NULL;
    collops->Exscan = NULL;
    
    MPIR_Process.comm_world->coll_fns = collops;
    MPIR_Process.comm_self->coll_fns = collops;
    

    /*
     * Let the channel perform any necessary initialization
     */
    mpi_errno = MPIDI_CH3_Init(has_args, has_env, &has_parent);
    if (mpi_errno != MPI_SUCCESS)
    {
	return mpi_errno;
    }

    /* MPIR_Process.parent_comm is set to a real communicator when the current
       process group is spawned by another MPI job using MPI_Comm_spawn or
       MPI_Comm_spawn_multiple. */
    if (has_parent)
    {
	/* XXX - functionality is not yet supported */
	MPIDI_err_printf(FCNAME, "initialization of parent communicator is "
			"UNIMPLEMENTED");
	MPID_Abort(NULL, MPI_ERR_INTERN);
    }
    
    /* MT - only single threaded applications are supported at the moment... */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_SINGLE;
    }

    MPIDI_dbg_printf(10, FCNAME, "exiting");
    return mpi_errno;
}
