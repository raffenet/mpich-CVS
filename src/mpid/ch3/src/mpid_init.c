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

    MPIDI_dbg_printf(10, FCNAME, "entering");
    
    MPIDI_Process.recv_posted_head = NULL;
    MPIDI_Process.recv_posted_tail = NULL;
    MPIDI_Process.recv_unexpected_head = NULL;
    MPIDI_Process.recv_unexpected_tail = NULL;
    
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
	assert(!has_parent);
	abort();  /* XXX - this functionality is not yet supported; also this
		     should be a MPID_Abort() but we don't have support for
		     that yet either.  Fortunately, every process should
		     fail the same condition. */
    }
    
    /* MT - only single threaded applications are supported at the moment... */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_SINGLE;
    }

    MPIDI_dbg_printf(10, FCNAME, "exiting");
    return mpi_errno;
}
