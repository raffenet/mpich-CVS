/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_START_EPOCH
        {
            mpi_errno = MPIDI_CH3_Start_epoch(NULL, MPIDI_CH3_ACCESS_AND_EXPOSURE_EPOCH, 
                                                  assert, win_ptr);
        }
#       endif
    }
    else {

        int comm_size, done, *recvcnts;
        int *rma_target_proc, *nops_to_proc, i, total_op_count, *curr_ops_cnt;
        MPIDI_RMA_ops *curr_ptr, *next_ptr;
        MPID_Comm *comm_ptr;
        MPID_Request **requests=NULL; /* array of requests */
        MPI_Win source_win_handle, target_win_handle;
        MPIDI_RMA_dtype_info *dtype_infos=NULL;
        void **dataloops=NULL;    /* to store dataloops for each datatype */
        MPID_Progress_state progress_state;
        
        /* In case this process was previously the target of passive target rma
         * operations, we need to take care of the following...
         * Since we allow MPI_Win_unlock to return without a done ack from
         * the target in the case of multiple rma ops and exclusive lock,
         * we need to check whether there is a lock on the window, and if
         * there is a lock, poke the progress engine until the operartions
         * have completed and the lock is released. */
        if (win_ptr->current_lock_type != MPID_LOCK_NONE)
        {
            MPID_Progress_start(&progress_state);
            while (win_ptr->current_lock_type != MPID_LOCK_NONE)
            {
                /* poke the progress engine */
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    MPID_Progress_end(&progress_state);
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail",
                                                     "**fail %s", "making progress on the rma messages failed");
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
            }
            MPID_Progress_end(&progress_state);
        }
        
        if (assert & MPI_MODE_NOPRECEDE)
        {
            win_ptr->fence_cnt = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;
            goto fn_exit;
        }
        
        if ((win_ptr->fence_cnt == 0) && ((assert & MPI_MODE_NOSUCCEED) != 1))
        {
            /* win_ptr->fence_cnt == 0 means either this is the very first
               call to fence or the preceding fence had the
               MPI_MODE_NOSUCCEED assert. 
               Do nothing except increment the count. */
            win_ptr->fence_cnt = 1;
        }
        else
        {
            /* This is the second or later fence. Do all the preceding RMA ops. */
            
            MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
            
            /* First inform every process whether it is a target of RMA
               ops from this process */
            comm_size = comm_ptr->local_size;
            rma_target_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
            /* --BEGIN ERROR HANDLING-- */
            if (!rma_target_proc)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
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
            
            /* set rma_target_proc[i] to 1 if rank i is a target of RMA
               ops from this process */
            total_op_count = 0;
            curr_ptr = win_ptr->rma_ops_list;
            while (curr_ptr != NULL)
            {
                total_op_count++;
                rma_target_proc[curr_ptr->target_rank] = 1;
                nops_to_proc[curr_ptr->target_rank]++;
                curr_ptr = curr_ptr->next;
            }
            
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
                requests = (MPID_Request **) MPIU_Malloc(total_op_count *
                                                         sizeof(MPID_Request*));
                /* --BEGIN ERROR HANDLING-- */
                if (!requests)
                {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
                
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
            
            /* do a reduce_scatter (with MPI_SUM) on rma_target_proc. As a result,
               each process knows how many other processes will be doing
               RMA ops on its window */  
            
            /* first initialize the completion counter. */
            win_ptr->my_counter = comm_size;
            
            /* set up the recvcnts array for reduce scatter */
            
            recvcnts = (int *) MPIU_Malloc(comm_size * sizeof(int));
            /* --BEGIN ERROR HANDLING-- */
            if (!recvcnts)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            for (i=0; i<comm_size; i++)
            {
                recvcnts[i] = 1;
            }
            
            MPIR_Nest_incr();
            mpi_errno = NMPI_Reduce_scatter(MPI_IN_PLACE, rma_target_proc, recvcnts,
                                            MPI_INT, MPI_SUM, win_ptr->comm);
            /* result is stored in rma_target_proc[0] */
            MPIR_Nest_decr();
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "The reduce_scatter to send out the data to all the nodes in the fence failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            /* Set the completion counter */
            /* FIXME: MT: this needs to be done atomically because other
               procs have the address and could decrement it. */
            win_ptr->my_counter = win_ptr->my_counter - comm_size +
                rma_target_proc[0];  
            
            MPIU_Free(recvcnts);
            MPIU_Free(rma_target_proc);
            
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
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "sending the rma message failed");
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
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "receiving the rma message failed");
                        goto fn_exit;
                    }
                    /* --END ERROR HANDLING-- */
                    break;
                default:
                    /* --BEGIN ERROR HANDLING-- */
                    /* FIXME - return some error code here */
                    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "invalid RMA operation");
                    goto fn_exit;
                    /* --END ERROR HANDLING-- */
                }
                i++;
                curr_ops_cnt[curr_ptr->target_rank]++;
                curr_ptr = curr_ptr->next;
            }
            
            MPIU_Free(nops_to_proc);
            MPIU_Free(curr_ops_cnt);
            
            
            if (total_op_count)
            { 
                done = 1;
                MPID_Progress_start(&progress_state);
                while (total_op_count)
                {
                    for (i=0; i<total_op_count; i++)
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
                                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                                     "**fail", "**fail %s", "rma message operation failed");
                                    goto fn_exit;
                                }
                                /* --END ERROR HANDLING-- */
                                /* if origin datatype was a derived
                                   datatype, it will get freed when the
                                   request gets freed. */ 
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
                    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno != MPI_SUCCESS)
                    {
                        MPID_Progress_end(&progress_state);
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail",
                                                         "**fail %s", "making progress on the rma messages failed");
                        goto fn_exit;
                    }
                    /* --END ERROR HANDLING-- */
                    
                    done = 1;
                } 
                MPID_Progress_end(&progress_state);
            }
            
            if (total_op_count != 0)
            {
                MPIU_Free(requests);
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
            
            /* wait for all operations from other processes to finish */
            if (win_ptr->my_counter)
            {
                MPID_Progress_start(&progress_state);
                while (win_ptr->my_counter)
                {
                    mpi_errno = MPID_Progress_wait(&progress_state);
                    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno != MPI_SUCCESS)
                    {
                        MPID_Progress_end(&progress_state);
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail",
                                                         "**fail %s", "making progress on the rma messages failed");
                        goto fn_exit;
                    }
                    /* --END ERROR HANDLING-- */
                }
                MPID_Progress_end(&progress_state);
            } 
            
            if (assert & MPI_MODE_NOSUCCEED)
            {
                win_ptr->fence_cnt = 0;
            }
        }

    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Send_rma_msg(MPIDI_RMA_ops *rma_op, MPID_Win *win_ptr,
                            MPI_Win source_win_handle, MPI_Win target_win_handle, 
                            MPIDI_RMA_dtype_info *dtype_info, 
                            void **dataloop, MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_put_t *put_pkt = &upkt.put;
    MPIDI_CH3_Pkt_accum_t *accum_pkt = &upkt.accum;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno=MPI_SUCCESS;
    int origin_dt_derived, target_dt_derived, origin_type_size, iovcnt, iov_n; 
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Datatype *target_dtp=NULL, *origin_dtp=NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);

    if (rma_op->type == MPIDI_RMA_PUT)
    {
        MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
        put_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;

        put_pkt->count = rma_op->target_count;
        put_pkt->datatype = rma_op->target_datatype;
        put_pkt->dataloop_size = 0;
        put_pkt->target_win_handle = target_win_handle;
        put_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (void*) put_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*put_pkt);
    }
    else
    {
        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
        accum_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        accum_pkt->count = rma_op->target_count;
        accum_pkt->datatype = rma_op->target_datatype;
        accum_pkt->dataloop_size = 0;
        accum_pkt->op = rma_op->op;
        accum_pkt->target_win_handle = target_win_handle;
        accum_pkt->source_win_handle = source_win_handle;

        iov[0].MPID_IOV_BUF = (void*) accum_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*accum_pkt);
    }

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/

    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    if (HANDLE_GET_KIND(rma_op->origin_datatype) != HANDLE_KIND_BUILTIN)
    {
        origin_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->origin_datatype, origin_dtp);
    }
    else
    {
        origin_dt_derived = 0;
    }

    if (HANDLE_GET_KIND(rma_op->target_datatype) != HANDLE_KIND_BUILTIN)
    {
        target_dt_derived = 1;
        MPID_Datatype_get_ptr(rma_op->target_datatype, target_dtp);
    }
    else
    {
        target_dt_derived = 0;
    }

    if (target_dt_derived)
    {
        /* derived datatype on target. fill derived datatype info */
        dtype_info->is_contig = target_dtp->is_contig;
        dtype_info->n_contig_blocks = target_dtp->n_contig_blocks;
        dtype_info->size = target_dtp->size;
        dtype_info->extent = target_dtp->extent;
        dtype_info->dataloop_size = target_dtp->dataloop_size;
        dtype_info->dataloop_depth = target_dtp->dataloop_depth;
        dtype_info->eltype = target_dtp->eltype;
        dtype_info->dataloop = target_dtp->dataloop;
        dtype_info->ub = target_dtp->ub;
        dtype_info->lb = target_dtp->lb;
        dtype_info->true_ub = target_dtp->true_ub;
        dtype_info->true_lb = target_dtp->true_lb;
        dtype_info->has_sticky_ub = target_dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = target_dtp->has_sticky_lb;

        *dataloop = MPIU_Malloc(target_dtp->dataloop_size);
	/* --BEGIN ERROR HANDLING-- */
        if (!(*dataloop))
	{
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        memcpy(*dataloop, target_dtp->dataloop, target_dtp->dataloop_size);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        if (rma_op->type == MPIDI_RMA_PUT)
	{
            put_pkt->dataloop_size = target_dtp->dataloop_size;
	}
        else
	{
            accum_pkt->dataloop_size = target_dtp->dataloop_size;
	}
    }

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!origin_dt_derived)
    {
	/* basic datatype on origin */
        if (!target_dt_derived)
	{
	    /* basic datatype on target */
            iov[1].MPID_IOV_BUF = rma_op->origin_addr;
            iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
            iovcnt = 2;
        }
        else
	{
	    /* derived datatype on target */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
            iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
            iov[2].MPID_IOV_BUF = *dataloop;
            iov[2].MPID_IOV_LEN = target_dtp->dataloop_size;

            iov[3].MPID_IOV_BUF = rma_op->origin_addr;
            iov[3].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
            iovcnt = 4;
        }

        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, request);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
    }
    else
    {
	/* derived datatype on origin */

        if (!target_dt_derived)
	{
	    /* basic datatype on target */
            iovcnt = 1;
	}
        else
	{
	    /* derived datatype on target */
            iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
            iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
            iov[2].MPID_IOV_BUF = *dataloop;
            iov[2].MPID_IOV_LEN = target_dtp->dataloop_size;
            iovcnt = 3;
        }

        *request = MPID_Request_create();
        if (*request == NULL) {
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
	    /* --END ERROR HANDLING-- */
        }

        MPIU_Object_set_ref(*request, 2);
        (*request)->kind = MPID_REQUEST_SEND;
	    
        (*request)->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
           is freed. */ 

        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype,
                          &((*request)->dev.segment), 0);
        (*request)->dev.segment_first = 0;
        (*request)->dev.segment_size = rma_op->origin_count * origin_type_size;
	    
        iov_n = MPID_IOV_LIMIT - iovcnt;
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(*request,
                                                     &iov[iovcnt],
                                                     &iov_n); 
        if (mpi_errno == MPI_SUCCESS)
        {
            iov_n += iovcnt;
            
            mpi_errno = MPIDI_CH3_iSendv(vc, *request, iov, iov_n);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                MPID_Datatype_release((*request)->dev.datatype_ptr);
                MPIU_Object_set_ref(*request, 0);
                MPIDI_CH3_Request_destroy(*request);
                *request = NULL;
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
		MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
        }
        else
        {
	    /* --BEGIN ERROR HANDLING-- */
            MPID_Datatype_release((*request)->dev.datatype_ptr);
            MPIU_Object_set_ref(*request, 0);
            MPIDI_CH3_Request_destroy(*request);
            *request = NULL;
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
            return mpi_errno;
	    /* --END ERROR HANDLING-- */
        }
    }

    if (target_dt_derived)
    {
        MPID_Datatype_release(target_dtp);
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_RMA_MSG);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Recv_rma_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Recv_rma_msg(MPIDI_RMA_ops *rma_op, MPID_Win *win_ptr,
                            MPI_Win source_win_handle, MPI_Win target_win_handle, 
                            MPIDI_RMA_dtype_info *dtype_info, void **dataloop, 
                            MPID_Request **request) 
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_t *get_pkt = &upkt.get;
    int mpi_errno;
    MPIDI_VC_t * vc;
    MPID_Comm *comm_ptr;
    MPID_Request *req = NULL;
    MPID_Datatype *dtp;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);

    /* create a request, store the origin buf, cnt, datatype in it,
       and pass a handle to it in the get packet. When the get
       response comes from the target, it will contain the request
       handle. */  
    req = MPID_Request_create();
    if (req == NULL) {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }

    *request = req;

    MPIU_Object_set_ref(req, 2);

    req->dev.user_buf = rma_op->origin_addr;
    req->dev.user_count = rma_op->origin_count;
    req->dev.datatype = rma_op->origin_datatype;
    req->dev.target_win_handle = MPI_WIN_NULL;
    req->dev.source_win_handle = source_win_handle;
    if (HANDLE_GET_KIND(req->dev.datatype) != HANDLE_KIND_BUILTIN)
    {
        MPID_Datatype_get_ptr(req->dev.datatype, dtp);
        req->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
           request is freed. */  
    }

    MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
    get_pkt->addr = (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
    get_pkt->count = rma_op->target_count;
    get_pkt->datatype = rma_op->target_datatype;
    get_pkt->request_handle = req->handle;
    get_pkt->target_win_handle = target_win_handle;
    get_pkt->source_win_handle = source_win_handle;

/*    printf("send pkt: type %d, addr %d, count %d, base %d\n", rma_pkt->type,
           rma_pkt->addr, rma_pkt->count, win_ptr->base_addrs[rma_op->target_rank]);
    fflush(stdout);
*/
	    
    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    if (HANDLE_GET_KIND(rma_op->target_datatype) ==
        HANDLE_KIND_BUILTIN)
    {
        /* basic datatype on target. simply send the get_pkt. */
        mpi_errno = MPIDI_CH3_iStartMsg(vc, get_pkt, sizeof(*get_pkt), &req);
    }
    else
    {
        /* derived datatype on target. fill derived datatype info and
           send it along with get_pkt. */

        MPID_Datatype_get_ptr(rma_op->target_datatype, dtp);
        dtype_info->is_contig = dtp->is_contig;
        dtype_info->n_contig_blocks = dtp->n_contig_blocks;
        dtype_info->size = dtp->size;
        dtype_info->extent = dtp->extent;
        dtype_info->dataloop_size = dtp->dataloop_size;
        dtype_info->dataloop_depth = dtp->dataloop_depth;
        dtype_info->eltype = dtp->eltype;
        dtype_info->dataloop = dtp->dataloop;
        dtype_info->ub = dtp->ub;
        dtype_info->lb = dtp->lb;
        dtype_info->true_ub = dtp->true_ub;
        dtype_info->true_lb = dtp->true_lb;
        dtype_info->has_sticky_ub = dtp->has_sticky_ub;
        dtype_info->has_sticky_lb = dtp->has_sticky_lb;

        *dataloop = MPIU_Malloc(dtp->dataloop_size);
	/* --BEGIN ERROR HANDLING-- */
        if (!(*dataloop))
	{
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        memcpy(*dataloop, dtp->dataloop, dtp->dataloop_size);
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        get_pkt->dataloop_size = dtp->dataloop_size;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)get_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)dtype_info;
        iov[1].MPID_IOV_LEN = sizeof(*dtype_info);
        iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)*dataloop;
        iov[2].MPID_IOV_LEN = dtp->dataloop_size;
        
        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, 3, &req);

        /* release the target datatype */
        MPID_Datatype_release(dtp);
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
	MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* release the request returned by iStartMsg or iStartMsgv */
    if (req != NULL)
    {
        MPID_Request_release(req);
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RECV_RMA_MSG);
    return mpi_errno;
}
