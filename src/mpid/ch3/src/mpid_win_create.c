/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info
                    *info_ptr, MPID_Comm *comm_ptr, MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);

    return mpi_errno;
}
