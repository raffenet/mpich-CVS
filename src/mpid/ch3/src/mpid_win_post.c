/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_post
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_post(MPID_Group *group_ptr, int assert, MPID_Win *win_ptr)
{
    int nest_level_inc = FALSE;
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_POST);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_POST);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_START_EPOCH
        {
            mpi_errno = MPIDI_CH3_Start_epoch(group_ptr, MPIDI_CH3_EXPOSURE_EPOCH, 
                                                  assert, win_ptr);
        }
#       endif
    }
    else {
        MPI_Group win_grp, post_grp;
        int i, post_grp_size, *ranks_in_post_grp, *ranks_in_win_grp, dst, rank;

        /* Reset the fence counter so that in case the user has switched from fence to 
           post-wait synchronization, he cannot use the previous fence to mark the beginning 
           of a fence epoch.  */
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
        
        post_grp_size = group_ptr->size;
        
        /* initialize the completion counter */
        win_ptr->my_counter = post_grp_size;
        
        if ((assert & MPI_MODE_NOCHECK) == 0)
        {
            /* NOCHECK not specified. We need to notify the source
               processes that Post has been called. */  
            
            /* We need to translate the ranks of the processes in
               post_group to ranks in win_ptr->comm, so that we
               can do communication */
            
            ranks_in_post_grp = (int *) MPIU_Malloc(post_grp_size * sizeof(int));
            /* --BEGIN ERROR HANDLING-- */
            if (!ranks_in_post_grp)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            ranks_in_win_grp = (int *) MPIU_Malloc(post_grp_size * sizeof(int));
            /* --BEGIN ERROR HANDLING-- */
            if (!ranks_in_win_grp)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        
            for (i=0; i<post_grp_size; i++)
            {
                ranks_in_post_grp[i] = i;
            }
        
	    nest_level_inc = TRUE;
            MPIR_Nest_incr();
            
            mpi_errno = NMPI_Comm_group(win_ptr->comm, &win_grp);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            post_grp = group_ptr->handle;

            mpi_errno = NMPI_Group_translate_ranks(post_grp, post_grp_size,
                                       ranks_in_post_grp, win_grp, ranks_in_win_grp);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            NMPI_Comm_rank(win_ptr->comm, &rank);

            /* Send a 0-byte message to the source processes */
            for (i=0; i<post_grp_size; i++)
            {
                dst = ranks_in_win_grp[i];

                if (dst != rank) {
                    mpi_errno = NMPI_Send(&i, 0, MPI_INT, dst, 100, win_ptr->comm);
                    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno)
                    {
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                        goto fn_exit;
                    }
                    /* --END ERROR HANDLING-- */
                }
            }
            
            MPIU_Free(ranks_in_win_grp);
            MPIU_Free(ranks_in_post_grp);

            mpi_errno = NMPI_Group_free(&win_grp);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }    
    }

 fn_exit:
    if (nest_level_inc)
    { 
	MPIR_Nest_decr();
    }
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_POST);
    return mpi_errno;
}
