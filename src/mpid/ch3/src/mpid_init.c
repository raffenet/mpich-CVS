/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

MPIDI_Process_t MPIDI_Process;

#undef FUNCNAME
#define FUNCNAME MPID_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Init(int * argc, char *** argv, int requested, int * provided, int * has_args, int * has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));
    
    MPIDI_Process.recv_posted_head = NULL;
    MPIDI_Process.recv_posted_tail = NULL;
    MPIDI_Process.recv_unexpected_head = NULL;
    MPIDI_Process.recv_unexpected_tail = NULL;

    /*
     * Initialize the collective operations for the MPI_COMM_WORLD and MPI_COMM_SELF
     *
     * NOTE: now using the default collective operations supplied by MPICH
     */
#   if FALSE
    {
	MPID_Collops * collops;
	
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
    }
#   endif
    
    /*
     * Set process attributes.  These can be overridden by the channel if necessary.
     */
    MPIR_Process.attrs.tag_ub          = INT_MAX;
    
#   if defined(HAVE_GETHOSTNAME)
    {
	/* The value 128 is returned by the ch3/Makefile for the echomaxprocname target.  */
	MPIDI_Process.processor_name = MPIU_Malloc(128);
	if(gethostname(MPIDI_Process.processor_name, 128) != 0)
	{
	    MPIU_Free(MPIDI_Process.processor_name);
	    MPIDI_Process.processor_name = NULL;
	}
    }
#   else
    {
	MPIDI_Process.processor_name = NULL;
    }
#   endif
    
    /*
     * Let the channel perform any necessary initialization
     */
    mpi_errno = MPIDI_CH3_Init(has_args, has_env, &has_parent);
    if (mpi_errno != MPI_SUCCESS)
    {
	return mpi_errno;
    }

    /* MPIR_Process.parent_comm is set to a real communicator when the current process group is spawned by another MPI job using
       MPI_Comm_spawn or MPI_Comm_spawn_multiple. */
    if (has_parent)
    {
	/* FIXME: functionality is not yet supported */
	MPIDI_ERR_PRINTF((FCNAME, "initialization of parent communicator is UNIMPLEMENTED"));
	MPID_Abort(NULL, MPI_ERR_INTERN);
    }
    
    /* MT:  only single threaded applications are supported at the moment... */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_SINGLE;
    }

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    return mpi_errno;
}
