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

    MPIR_Nest_incr();

    if (access_or_exposure == MPIDI_CH3_ACCESS_AND_EXPOSURE_EPOCH) {
        /* this is a win_fence. just do a barrier. */

        mpi_errno = NMPI_Barrier(win_ptr->comm);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
    }
    else {
        /* this is a win_start or win_post. */

        MPI_Group win_grp, grp;
        int i, grp_size, *ranks_in_grp, *ranks_in_win, dst, src, rank;
        volatile char *pscw_sync_addr;

        /* First translate the ranks of the processes in
           group_ptr to ranks in win_ptr->comm. Save the translated ranks
           and group_ptr in the win object because they will be needed in 
           end_epoch. */
        
        grp_size = group_ptr->size;
            
        ranks_in_grp = (int *) MPIU_Malloc(grp_size * sizeof(int));
        /* --BEGIN ERROR HANDLING-- */
        if (!ranks_in_grp)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        ranks_in_win = (int *) MPIU_Malloc(grp_size * sizeof(int));
        /* --BEGIN ERROR HANDLING-- */
        if (!ranks_in_win)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        for (i=0; i<grp_size; i++)
        {
            ranks_in_grp[i] = i;
        }
        
        grp = group_ptr->handle;
        
        mpi_errno = NMPI_Comm_group(win_ptr->comm, &win_grp);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        mpi_errno = NMPI_Group_translate_ranks(grp, grp_size, ranks_in_grp, win_grp, ranks_in_win);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        MPIU_Free(ranks_in_grp);
        
        if ((assert & MPI_MODE_NOCHECK) == 0) {
            
            /* MPI_MODE_NOCHECK not specified. Synchronization is necessary. */

            NMPI_Comm_rank(win_ptr->comm, &rank);

            if (access_or_exposure == MPIDI_CH3_ACCESS_EPOCH) {
                /* this is a Win_start. Since MPI_MODE_NOCHECK was not specified, 
                   we need to check if Win_post was called on the target processes. 
                   Wait for a 0-byte sync  message from each target process */


                for (i=0; i<grp_size; i++)
                {
                    src = ranks_in_win[i];
                    if (src != rank) {

                        /* Wait until a '1' is written in the sync array by the target 
                           at the location indexed by the rank of the target */

                        pscw_sync_addr = win_ptr->pscw_shm_structs[rank].addr;
                        while (1) {
                            if (pscw_sync_addr[src] == '1') {
                                /* reset it and break */
                                pscw_sync_addr[src] = '0';
                                break;
                            }
                        }

/*                        mpi_errno = NMPI_Recv(NULL, 0, MPI_INT, src, 100,
                          win_ptr->comm, MPI_STATUS_IGNORE); */
                        /* --BEGIN ERROR HANDLING-- */
                        if (mpi_errno)
                        {
                            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                            goto fn_exit;
                        }
                        /* --END ERROR HANDLING-- */
                    }
                }
            }

            else {  /* (access_or_exposure == MPIDI_CH3_EXPOSURE_EPOCH) */
                /* This is a Win_post. Since NOCHECK was not specified. We need to notify the 
                   source processes that Post has been called. */  
                        
                /* Send a 0-byte message to the origin processes */
                for (i=0; i<grp_size; i++)
                {
                    dst = ranks_in_win[i];
                    if (dst != rank) {
                        /* Write a '1' in the sync array of the origin process at the location
                           indexed by the rank of this process */

                        pscw_sync_addr = win_ptr->pscw_shm_structs[dst].addr;
                        pscw_sync_addr[rank] = '1';

/*                        mpi_errno = NMPI_Send(&i, 0, MPI_INT, dst, 100, win_ptr->comm); */
                        /* --BEGIN ERROR HANDLING-- */
                        if (mpi_errno)
                        {
                            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                            goto fn_exit;
                        }
                        /* --END ERROR HANDLING-- */

                    }
                }
            }
        }    

        /* save the ranks_in_win and group_ptr in win object */
        if (access_or_exposure == MPIDI_CH3_ACCESS_EPOCH) {
            win_ptr->access_epoch_grp_ranks_in_win = ranks_in_win;
            win_ptr->access_epoch_grp_ptr = group_ptr;
        }
        else {
            win_ptr->exposure_epoch_grp_ranks_in_win = ranks_in_win;
            win_ptr->exposure_epoch_grp_ptr = group_ptr;
        }

        MPIU_Object_add_ref( group_ptr );
    }

 fn_exit:
    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_START_EPOCH);
    return mpi_errno;
}

