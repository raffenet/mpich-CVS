/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_End_epoch()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_End_epoch
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_End_epoch(int access_or_exposure, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, src, dst, *ranks_in_win, grp_size, rank, comm_size;
    volatile char *pscw_sync_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_END_EPOCH);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_END_EPOCH);

    MPIR_Nest_incr();

    NMPI_Comm_rank(win_ptr->comm, &rank);
    NMPI_Comm_size(win_ptr->comm, &comm_size);

    if (access_or_exposure == MPIDI_CH3_ACCESS_EPOCH) {
        /* this is a Win_complete. Send a 0-byte sync message to each target process */

        grp_size = win_ptr->access_epoch_grp_ptr->size;
        ranks_in_win = win_ptr->access_epoch_grp_ranks_in_win;

        for (i=0; i<grp_size; i++)
        {
            dst = ranks_in_win[i];
            if (dst != rank) {

                /* Write a '1' in the sync array of the target process at the location
                   indexed by the rank of this process */
                
                pscw_sync_addr = (char *) win_ptr->pscw_shm_structs[dst].addr + comm_size;
                pscw_sync_addr[rank] = '1';

/*                mpi_errno = NMPI_Send(&i, 0, MPI_INT, dst, 100, win_ptr->comm);
 */
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

    else if (access_or_exposure == MPIDI_CH3_EXPOSURE_EPOCH) { 
        /* This is a Win_wait. */
                        
        grp_size = win_ptr->exposure_epoch_grp_ptr->size;
        ranks_in_win = win_ptr->exposure_epoch_grp_ranks_in_win;

        /* Recv a 0-byte message from the origin processes */
        for (i=0; i<grp_size; i++)
        {
            src = ranks_in_win[i];
            if (src != rank) {

                /* Wait until a '1' is written in the sync array by the origin
                   at the location indexed by the rank of the origin */

                pscw_sync_addr = (char *) win_ptr->pscw_shm_structs[rank].addr + comm_size;
                while (1) {
                    if (pscw_sync_addr[src] == '1') {
                        /* reset it and break */
                        pscw_sync_addr[src] == '0';
                        break;
                    }
                }



/*                mpi_errno = NMPI_Recv(NULL, 0, MPI_INT, src, 100, win_ptr->comm, MPI_STATUS_IGNORE);
 */
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

    else {
        /* return error */
    }

    if (access_or_exposure == MPIDI_CH3_ACCESS_EPOCH) {
        MPIU_Free(win_ptr->access_epoch_grp_ranks_in_win);
        MPIR_Group_release(win_ptr->access_epoch_grp_ptr);
    }
    else {
        MPIU_Free(win_ptr->exposure_epoch_grp_ranks_in_win);
        MPIR_Group_release(win_ptr->exposure_epoch_grp_ptr);
    }

 fn_exit:
    MPIR_Nest_decr();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_END_EPOCH);
    return mpi_errno;
}
