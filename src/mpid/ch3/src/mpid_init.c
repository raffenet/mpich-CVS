/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Init(int * argc, char *** argp, int requested, int * provided,
	      int * has_args, int * has_env)
{
    int mpi_errno = MPI_SUCCESS;
    int has_parent;
    
    mpi_errno = MPID_CH3_Init(has_args, has_env, &has_parent);
    if (mpi_errno != MPI_SUCCESS)
    {
	return mpi_errno;
    }

    /* MPIR_Process.parent_comm is set to a real communicator when the current
       process group is spawned by another MPI job using MPI_Comm_spawn or
       MPI_Comm_spawn_multiple. */
    if (has_parent)
    {
	abort();  /* XXX - this functionality is not yet supported; also this
		     should be a MPID_Abort() but we don't have support for
		     that yet either... */
    }
    
    /* only single threaded applications are supported for the moment... */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_SINGLE;
    }

    return mpi_errno;
}
