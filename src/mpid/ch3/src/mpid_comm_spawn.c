/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* 
 * MPID_Comm_spawn()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info,
		    int root, MPID_Comm *comm, MPID_Comm *intercomm,
		    int array_of_errcodes[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_SPAWN);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

#   if defined(MPIDI_CH3_Comm_spawn)
    {
	mpi_errno = MPIDI_CH3_Comm_spawn(command, argv, maxprocs, info, root,
					 comm, intercomm, array_of_errcodes);
    }
#   else
    {
	mpi_errno = MPI_ERR_INTERN;
    }
#   endif    

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_SPAWN);
    return mpi_errno;
}

