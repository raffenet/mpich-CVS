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
int MPID_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, int root,
		    MPID_Comm *comm, MPID_Comm *intercomm, int array_of_errcodes[])
{
    int mpi_errno = MPI_SUCCESS;
    char spawned_kvsname[80];

    MPIDI_STATE_DECL(MPID_STATE_MPID_SPAWN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SPAWN);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

/*  begin experimental Ralph and Rusty version */

    fprintf( stderr, "MPID_Comm_spawn entered, command=%s\n", command );

    PMI_Spawn(command, argv, maxprocs, spawned_kvsname);
    fprintf( stderr, "MPID_Comm_spawn: spawned_kvsname=%s\n", spawned_kvsname );

    fprintf( stderr, "MPID_Comm_spawn exited\n" );

/*  end experimental Ralph and Rusty version */

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SPAWN);
    return mpi_errno;
}
