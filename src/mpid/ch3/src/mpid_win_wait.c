/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_wait(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_WAIT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_WAIT);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_END_EPOCH
        {
            mpi_errno = MPIDI_CH3_End_epoch(MPIDI_CH3_EXPOSURE_EPOCH, win_ptr);
        }
#       endif
    }
    else {
        /* wait for all operations from other processes to finish */
        if (win_ptr->my_counter)
        {
            MPID_Progress_state progress_state;
            
            MPID_Progress_start(&progress_state);
            while (win_ptr->my_counter)
            {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    MPID_Progress_end(&progress_state);
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
                    return mpi_errno;
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        } 
        
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_WAIT);
    return mpi_errno;
}
