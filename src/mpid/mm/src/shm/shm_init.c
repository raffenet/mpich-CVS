/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "shmimpl.h"

SHM_PerProcess SHM_Process;

/*@
   shm_init - initialize the shared memory method

   Notes:
@*/
int shm_init( void )
{
#ifdef HAVE_WINDOWS_H
    DWORD n = 100;
    MPIDI_STATE_DECL(MPID_STATE_SHM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SHM_INIT);

    GetComputerName(SHM_Process.host, &n);
#else
    MPIDI_STATE_DECL(MPID_STATE_SHM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_SHM_INIT);

    gethostname(SHM_Process.host, 100);
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_SHM_INIT);
    return MPI_SUCCESS;
}


/*@
   shm_finalize - finalize the shared memory method

   Notes:
@*/
int shm_finalize( void )
{
    MPIDI_STATE_DECL(MPID_STATE_SHM_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_SHM_FINALIZE);
    MPIDI_FUNC_EXIT(MPID_STATE_SHM_FINALIZE);
    return MPI_SUCCESS;
}
