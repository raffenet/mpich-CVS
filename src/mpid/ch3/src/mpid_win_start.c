/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_start
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_start(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_START);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_START);

#   if defined(MPIDI_CH3_IMPLEMENTS_START_EPOCH)
    {
	mpi_errno = MPIDI_CH3_Win_start_epoch(group_ptr, MPIDI_CH3I_ACCESS_EPOCH, 
                                              assert, win_ptr);
    }
#   else
    {
        /* Reset the fence counter so that in case the user has switched from fence to 
           start-complete synchronization, he cannot use the previous fence to mark the 
           beginning of a fence epoch.  */
        win_ptr->fence_cnt = 0;

        /* In case this process was previously the target of passive target rma
         * operations, we need to take care of the following...
         * Since we allow MPI_Win_unlock to return without a done ack from
         * the target in the case of multiple rma ops and exclusive lock,
         * we need to check whether there is a lock on the window, and if
         * there is a lock, poke the progress engine until the operations
         * have completed and the lock is therefore released. */
        if (win_ptr->current_lock_type != MPID_LOCK_NONE)
        {
            MPID_Progress_state progress_state;
            
            /* poke the progress engine */
            MPID_Progress_start(&progress_state);
            while (win_ptr->current_lock_type != MPID_LOCK_NONE)
            {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    MPID_Progress_end(&progress_state);
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                     "**fail", "**fail %s", "making progress on the rma messages failed");
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        }

        win_ptr->start_group_ptr = group_ptr;
        MPIU_Object_add_ref( group_ptr );
        win_ptr->start_assert = assert;
    }
#   endif

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_START);
    return mpi_errno;
}

