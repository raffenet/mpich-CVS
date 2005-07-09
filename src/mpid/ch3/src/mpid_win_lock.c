/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_lock(int lock_type, int dest, int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_LOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_LOCK);

    MPIU_UNREFERENCED_ARG(assert);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_START_PT_EPOCH
        {
            mpi_errno = MPIDI_CH3_Start_PT_epoch(lock_type, dest, assert, win_ptr);
        }
#       endif
    }
    else {
        MPIDI_RMA_ops *new_ptr;
        MPID_Comm *comm_ptr;

#ifdef FOO
#ifdef MPICH_SINGLE_THREADED
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
        return mpi_errno;
#endif
#endif

        /* Reset the fence counter so that in case the user has switched from fence to 
           lock-unlock synchronization, he cannot use the previous fence to mark the beginning 
           of a fence epoch.  */
        win_ptr->fence_cnt = 0;

        if (dest == MPI_PROC_NULL) goto fn_exit;
        
        MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
        
        if (dest == comm_ptr->rank) {
            /* The target is this process itself. We must block until the lock
             * is acquired. */
            
            /* poke the progress engine until lock is granted */
            if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0)
            {
                MPID_Progress_state progress_state;
                
                MPID_Progress_start(&progress_state);
                while (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0) 
                {
                    mpi_errno = MPID_Progress_wait(&progress_state);
                    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno != MPI_SUCCESS)
                    {
                        MPID_Progress_end(&progress_state);
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                         "**fail", "**fail %s", "making progress on rma messages failed");
                        goto fn_exit;
                    }
                    /* --END ERROR HANDLING-- */
                }
                MPID_Progress_end(&progress_state);
            }
            /* local lock acquired. local puts, gets, accumulates will be done 
               directly without queueing. */
        }
        
        else {
            /* target is some other process. add the lock request to rma_ops_list */
            
            new_ptr = (MPIDI_RMA_ops *) MPIU_Malloc(sizeof(MPIDI_RMA_ops));
            /* --BEGIN ERROR HANDLING-- */
            if (!new_ptr)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            win_ptr->rma_ops_list = new_ptr;
        
            new_ptr->next = NULL;  
            new_ptr->type = MPIDI_RMA_LOCK;
            new_ptr->target_rank = dest;
            new_ptr->lock_type = lock_type;
        }
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_LOCK);
    return mpi_errno;
}
