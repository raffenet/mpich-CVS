/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Win_create()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_CREATE);


    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_CREATE);
    return mpi_errno;
}




