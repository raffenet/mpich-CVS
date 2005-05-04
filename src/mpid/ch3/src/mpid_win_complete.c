/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_complete(MPID_Win *win_ptr)
{
    int nest_level_inc = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_COMPLETE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_COMPLETE);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_END_EPOCH
        {
            mpi_errno = MPIDI_CH3_End_epoch(MPIDI_CH3_ACCESS_EPOCH, win_ptr);
        }
#       endif
    }
    else {
        int comm_size, *nops_to_proc, src, new_total_op_count;
        int i, j, dst, done, total_op_count, *curr_ops_cnt;
        MPIDI_RMA_ops *curr_ptr, *next_ptr;
        MPID_Comm *comm_ptr;
        MPID_Request **requests; /* array of requests */
        MPI_Win source_win_handle, target_win_handle;
        MPIDI_RMA_dtype_info *dtype_infos=NULL;
        void **dataloops=NULL;    /* to store dataloops for each datatype */
        MPI_Group win_grp, start_grp;
        int start_grp_size, *ranks_in_start_grp, *ranks_in_win_grp, rank;
        
        MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
        comm_size = comm_ptr->local_size;
        
        /* Translate the ranks of the processes in
           start_group to ranks in win_ptr->comm */
        
        start_grp_size = win_ptr->start_group_ptr->size;
        
        ranks_in_start_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
        /* --BEGIN ERROR HANDLING-- */
        if (!ranks_in_start_grp)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        ranks_in_win_grp = (int *) MPIU_Malloc(start_grp_size * sizeof(int));
        /* --BEGIN ERROR HANDLING-- */
        if (!ranks_in_win_grp)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        for (i=0; i<start_grp_size; i++)
        {
            ranks_in_start_grp[i] = i;
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
        
        start_grp = win_ptr->start_group_ptr->handle;

        mpi_errno = NMPI_Group_translate_ranks(start_grp, start_grp_size,
                                               ranks_in_start_grp, win_grp, ranks_in_win_grp);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        
        /* If MPI_MODE_NOCHECK was not specified, we need to check if
           Win_post was called on the target processes. Wait for a 0-byte sync
           message from each target process */
        if ((win_ptr->start_assert & MPI_MODE_NOCHECK) == 0)
        {
            NMPI_Comm_rank(win_ptr->comm, &rank);
            for (i=0; i<start_grp_size; i++)
            {
                src = ranks_in_win_grp[i];
                if (src != rank) {
                    mpi_errno = NMPI_Recv(NULL, 0, MPI_INT, src, 100,
                                          win_ptr->comm, MPI_STATUS_IGNORE);
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
        
        /* keep track of no. of ops to each proc. Needed for knowing
           whether or not to decrement the completion counter. The
           completion counter is decremented only on the last
           operation. */
        
        nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        /* --BEGIN ERROR HANDLING-- */
        if (!nops_to_proc)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        total_op_count = 0;
        curr_ptr = win_ptr->rma_ops_list;
        while (curr_ptr != NULL)
        {
            nops_to_proc[curr_ptr->target_rank]++;
            total_op_count++;
            curr_ptr = curr_ptr->next;
        }
        
        requests = (MPID_Request **) MPIU_Malloc((total_op_count+start_grp_size) *
                                                 sizeof(MPID_Request*));
        /* We allocate a few extra requests because if there are no RMA
           ops to a target process, we need to send a 0-byte message just
           to decrement the completion counter. */
        /* --BEGIN ERROR HANDLING-- */
        if (!requests)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        curr_ops_cnt = (int *) MPIU_Calloc(comm_size, sizeof(int));
        /* --BEGIN ERROR HANDLING-- */
        if (!curr_ops_cnt)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        if (total_op_count != 0)
        {
            dtype_infos = (MPIDI_RMA_dtype_info *)
                MPIU_Malloc(total_op_count*sizeof(MPIDI_RMA_dtype_info));
            /* --BEGIN ERROR HANDLING-- */
            if (!dtype_infos)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            dataloops = (void **) MPIU_Malloc(total_op_count*sizeof(void*));
            /* allocate one extra for use when receiving data. see below */
            /* --BEGIN ERROR HANDLING-- */
            if (!dataloops)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            for (i=0; i<total_op_count; i++)
            {
                dataloops[i] = NULL;
            }
        }
        
        i = 0;
        curr_ptr = win_ptr->rma_ops_list;
        while (curr_ptr != NULL)
        {
            /* The completion counter at the target is decremented only on 
               the last RMA operation. We indicate the last operation by 
               passing the source_win_handle only on the last operation. 
               Otherwise, we pass NULL */
            if (curr_ops_cnt[curr_ptr->target_rank] ==
                nops_to_proc[curr_ptr->target_rank] - 1) 
                source_win_handle = win_ptr->handle;
            else 
                source_win_handle = MPI_WIN_NULL;
            
            target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
            
            switch (curr_ptr->type)
            {
            case (MPIDI_RMA_PUT):
            case (MPIDI_RMA_ACCUMULATE):
                mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
                                                    source_win_handle, target_win_handle, &dtype_infos[i],
                                                    &dataloops[i], &requests[i]); 
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
                break;
            case (MPIDI_RMA_GET):
                mpi_errno = MPIDI_CH3I_Recv_rma_msg(curr_ptr, win_ptr,
                                                    source_win_handle, target_win_handle, &dtype_infos[i], 
                                                    &dataloops[i], &requests[i]);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
                break;
            default:
                /* FIXME - return some error code here */
                break;
            }
            i++;
            curr_ops_cnt[curr_ptr->target_rank]++;
            curr_ptr = curr_ptr->next;
        }
        
        /* If the start_group included some processes that did not end up
           becoming targets of  RMA operations from this process, we need
           to send a dummy message to those processes just to decrement
           the completion counter */
        
        j = i;
        new_total_op_count = total_op_count;
        for (i=0; i<start_grp_size; i++)
        {
            dst = ranks_in_win_grp[i];
            if (nops_to_proc[dst] == 0)
            {
                MPIDI_CH3_Pkt_t upkt;
                MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
                MPIDI_VC_t * vc;
                
                MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
                put_pkt->addr = NULL;
                put_pkt->count = 0;
                put_pkt->datatype = MPI_INT;
                put_pkt->target_win_handle = win_ptr->all_win_handles[dst];
                put_pkt->source_win_handle = win_ptr->handle;
                
                MPIDI_Comm_get_vc(comm_ptr, dst, &vc);
                
                mpi_errno = MPIDI_CH3_iStartMsg(vc, put_pkt,
                                                sizeof(*put_pkt),
                                                &requests[j]);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
                j++;
                new_total_op_count++;
            }
        }
        
        MPIU_Free(ranks_in_win_grp);
        MPIU_Free(ranks_in_start_grp);
        MPIU_Free(nops_to_proc);
        MPIU_Free(curr_ops_cnt);
        
        if (new_total_op_count)
        {
            MPID_Progress_state progress_state;
            
            done = 1;
            MPID_Progress_start(&progress_state);
            while (new_total_op_count)
            {
                for (i=0; i<new_total_op_count; i++)
                {
                    if (requests[i] != NULL)
                    {
                        if (*(requests[i]->cc_ptr) != 0)
                        {
                            done = 0;
                            break;
                        }
                        else
                        {
                            mpi_errno = requests[i]->status.MPI_ERROR;
                            /* --BEGIN ERROR HANDLING-- */
                            if (mpi_errno != MPI_SUCCESS)
                            {
                                MPID_Progress_end(&progress_state);
                                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                                 "**fail", 0);
                                goto fn_exit;
                            }
                            /* --END ERROR HANDLING-- */
                            MPID_Request_release(requests[i]);
                            requests[i] = NULL;
                        }
                    }
                }
                
                if (done)
                {
                    break;
                }
                
                mpi_errno = MPID_Progress_wait(&progress_state);
                done = 1;
            } 
            MPID_Progress_end(&progress_state);
        }
        
        MPIU_Free(requests);
        if (total_op_count != 0)
        {
            MPIU_Free(dtype_infos);
            for (i=0; i<total_op_count; i++)
            {
                if (dataloops[i] != NULL)
                {
                    MPIU_Free(dataloops[i]);
                }
            }
            MPIU_Free(dataloops);
        }
        
        /* free MPIDI_RMA_ops_list */
        curr_ptr = win_ptr->rma_ops_list;
        while (curr_ptr != NULL)
        {
            next_ptr = curr_ptr->next;
            MPIU_Free(curr_ptr);
            curr_ptr = next_ptr;
        }
        win_ptr->rma_ops_list = NULL;
    
        mpi_errno = NMPI_Group_free(&win_grp);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        /* free the group stored in window */
        MPIR_Group_release(win_ptr->start_group_ptr);
        win_ptr->start_group_ptr = NULL; 
    }

 fn_exit:
    if (nest_level_inc)
    { 
	MPIR_Nest_decr();
    }
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_COMPLETE);
    return mpi_errno;
}
