/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Start_epoch()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Start_epoch
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Start_epoch(MPID_Group *group_ptr, int access_or_exposure, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_START_EPOCH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_START_EPOCH);


    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_START_EPOCH);
    return mpi_errno;
}
