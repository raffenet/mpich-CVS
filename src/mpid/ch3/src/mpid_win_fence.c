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

#   if defined(MPIDI_CH3_IMPLEMENTS_START_EPOCH)
    {
	mpi_errno = MPIDI_CH3_Win_start_epoch(NULL, MPIDI_CH3I_ACCESS_AND_EXPOSURE_EPOCH, 
                                              assert, win_ptr);
    }
#   else
    {

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
#   endif

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
    return mpi_errno;
}


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




#ifdef OLDSTUFF
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

MPIDI_RMA_ops *MPIDI_RMA_ops_list=NULL;

#undef FUNCNAME
#define FUNCNAME MPID_Win_fence
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, *nops_to_proc, src, dest;
    int *nops_from_proc, rank, i, j, *tags, total_op_count, req_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIDI_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
    } MPIDI_RMA_op_info;
    MPIDI_RMA_op_info *rma_op_infos;
    typedef struct MPIDI_RMA_dtype_info { /* for derived datatypes */
        int           is_contig; 
        int           n_contig_blocks;
        int           size;     
        MPI_Aint      extent;   
        int           dataloop_size; 
        void          *dataloop;  /* pointer needed to update pointers
                                     within dataloop on remote side */
        int           dataloop_depth; 
        int           eltype;
        MPI_Aint ub, lb, true_ub, true_lb;
        int has_sticky_ub, has_sticky_lb;
    } MPIDI_RMA_dtype_info;
    MPIDI_RMA_dtype_info *dtype_infos;
    void **dataloops;    /* to store dataloops for each datatype */
    MPI_Request *reqs;
    MPI_User_function *uop;
    MPI_Op op;
    void *tmp_buf;
    MPI_Aint extent, ptrdiff, true_lb, true_extent;
    MPID_Datatype *dtp, *new_dtp=NULL;
    void *win_buf_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    if (assert & MPI_MODE_NOPRECEDE) {
        win_ptr->fence_cnt = (assert & MPI_MODE_NOSUCCEED) ? 0 : 1;

        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
        return MPI_SUCCESS;
    }

    if ((win_ptr->fence_cnt == 0) && ((assert & MPI_MODE_NOSUCCEED) != 1)) {
        /* win_ptr->fence_cnt == 0 means either this is the very first
           call to fence or the preceding fence had the
           MPI_MODE_NOSUCCEED assert. 
           Do nothing except increment the count. */
        win_ptr->fence_cnt = 1;
    }
    else {
        /* This is the second or later fence. Do all the preceding RMA ops. */

	MPIR_Nest_incr();

        comm = win_ptr->comm;
        NMPI_Comm_size(comm, &comm_size);
        NMPI_Comm_rank(comm, &rank);

        /* First, each process informs every other process how many
           RMA ops from this process is it the target for. */

        nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_to_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }
        nops_from_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_from_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        total_op_count = 0;
        curr_ptr = MPIDI_RMA_ops_list;
        while (curr_ptr != NULL) {
            nops_to_proc[curr_ptr->target_rank]++;
            total_op_count++;
            curr_ptr = curr_ptr->next;
        }
        
        NMPI_Alltoall(nops_to_proc, 1, MPI_INT, nops_from_proc, 1,
                     MPI_INT, comm);

        /* For each RMA op, first send the type (put or get), target
           displ, count, datatype. Then issue an isend for a 
           put or irecv for a get. */

        rma_op_infos = (MPIDI_RMA_op_info *) 
            MPIU_Malloc((total_op_count+1) * sizeof(MPIDI_RMA_op_info));
        /* allocate one extra for use in receiving info below */ 

        if (!rma_op_infos) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        reqs = (MPI_Request *)
            MPIU_Malloc(total_op_count*4*sizeof(MPI_Request)); 
        if (!reqs) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
        if (!tags) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        dtype_infos = (MPIDI_RMA_dtype_info *)
            MPIU_Malloc((total_op_count+1)*sizeof(MPIDI_RMA_dtype_info));
        /* allocate one extra for use when receiving data. see below */
        if (!dtype_infos) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

        dataloops = (void **) MPIU_Malloc((total_op_count+1)*sizeof(void*));
        /* allocate one extra for use when receiving data. see below */
        if (!dataloops) {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }
        for (i=0; i<=total_op_count; i++)
            dataloops[i] = NULL;

        i = 0;
        req_cnt = 0;
        curr_ptr = MPIDI_RMA_ops_list;
        while (curr_ptr != NULL) {
            rma_op_infos[i].type = curr_ptr->type;
            rma_op_infos[i].disp = curr_ptr->target_disp;
            rma_op_infos[i].count = curr_ptr->target_count;
            rma_op_infos[i].datatype = curr_ptr->target_datatype;
            rma_op_infos[i].op = curr_ptr->op; 
            dest = curr_ptr->target_rank;

            if (HANDLE_GET_KIND(curr_ptr->target_datatype) ==
                HANDLE_KIND_BUILTIN) {
                /* basic datatype. send only the rma_op_info struct */
                rma_op_infos[i].datatype_kind = MPIDI_RMA_DATATYPE_BASIC;
                /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
                mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                       sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;
            }
            else {
                /* derived datatype. send rma_op_info_struct as well
                   as derived datatype information */

                rma_op_infos[i].datatype_kind = MPIDI_RMA_DATATYPE_DERIVED; 
                /* fill derived datatype info */
                MPID_Datatype_get_ptr(curr_ptr->target_datatype, dtp);
                dtype_infos[i].is_contig = dtp->is_contig;
                dtype_infos[i].n_contig_blocks = dtp->n_contig_blocks;
                dtype_infos[i].size = dtp->size;
                dtype_infos[i].extent = dtp->extent;
                dtype_infos[i].dataloop_size = dtp->dataloop_size;
                dtype_infos[i].dataloop_depth = dtp->dataloop_depth;
                dtype_infos[i].eltype = dtp->eltype;
                dtype_infos[i].dataloop = dtp->dataloop;
                dtype_infos[i].ub = dtp->ub;
                dtype_infos[i].lb = dtp->lb;
                dtype_infos[i].true_ub = dtp->true_ub;
                dtype_infos[i].true_lb = dtp->true_lb;
                dtype_infos[i].has_sticky_ub = dtp->has_sticky_ub;
                dtype_infos[i].has_sticky_lb = dtp->has_sticky_lb;

                dataloops[i] = MPIU_Malloc(dtp->dataloop_size);
                if (!dataloops[i]) {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
		MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
                memcpy(dataloops[i], dtp->dataloop, dtp->dataloop_size);
		MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

                /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
                mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                       sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;

                /* send the datatype info */
                /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
                mpi_errno = NMPI_Isend(&dtype_infos[i],
                                       sizeof(MPIDI_RMA_dtype_info), MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;

                mpi_errno = NMPI_Isend(dataloops[i],
                                       dtp->dataloop_size, MPI_BYTE, 
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                req_cnt++;
                tags[dest]++;

                /* release the target dataype */
                MPID_Datatype_release(dtp);
            }

            /* now send or recv the data */

            if ((curr_ptr->type == MPIDI_RMA_PUT) ||
                (curr_ptr->type == MPIDI_RMA_ACCUMULATE))
                mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                       curr_ptr->origin_count,
                                       curr_ptr->origin_datatype,
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
            else
                mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                       curr_ptr->origin_count,
                                       curr_ptr->origin_datatype,
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
            if (mpi_errno) {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                return mpi_errno;
            }
            if (HANDLE_GET_KIND(curr_ptr->origin_datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
                MPID_Datatype_release(dtp);
            }

            req_cnt++;
            tags[dest]++;

            curr_ptr = curr_ptr->next;
            i++;
        }        


        /* Now for each op for which this process is a target, first
           get the info regarding that op and then post an isend or
           irecv to perform the operation. */

        for (i=0; i<comm_size; i++) {
            /* instead of having everyone start receiving from 0,
               stagger the recvs a bit */ 
            src = (rank + i) % comm_size;
            tags[src] = 0;  /* start from tag 0 */

            for (j=0; j<nops_from_proc[src]; j++) {
                /* recv the info about the RMA op into the extra
                   MPIDI_RMA_op_info allocated at the end of the
                   rma_op_infos array. */
                mpi_errno = NMPI_Recv(&rma_op_infos[total_op_count],
                                      sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) {
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                tags[src]++;

                if (rma_op_infos[total_op_count].datatype_kind ==
                    MPIDI_RMA_DATATYPE_DERIVED) {
                    /* recv the derived datatype info and create
                       derived datatype */
                    mpi_errno =
                        NMPI_Recv(&dtype_infos[total_op_count],
                                  sizeof(MPIDI_RMA_dtype_info),
                                  MPI_BYTE, src, tags[src], comm,
                                  MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    tags[src]++;

                    /* recv dataloop */
                    dataloops[total_op_count] = (void *)
                        MPIU_Malloc(dtype_infos[total_op_count].dataloop_size);
                    if (!dataloops[total_op_count]) {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }

                    mpi_errno = NMPI_Recv(dataloops[total_op_count],
                                          dtype_infos[total_op_count].dataloop_size,
                                          MPI_BYTE, src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    tags[src]++;

                    /* create derived datatype */

                    /* allocate new datatype object and handle */
                    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
                    if (!new_dtp) {
                        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    
                    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
                    MPIU_Object_set_ref(new_dtp, 1);
                    new_dtp->is_permanent = 0;
                    new_dtp->is_committed = 1;
                    new_dtp->attributes   = 0;
                    new_dtp->cache_id     = 0;
                    new_dtp->name[0]      = 0;
                    new_dtp->is_contig = dtype_infos[total_op_count].is_contig;
                    new_dtp->n_contig_blocks =
                        dtype_infos[total_op_count].n_contig_blocks; 
                    new_dtp->size = dtype_infos[total_op_count].size;
                    new_dtp->extent = dtype_infos[total_op_count].extent;
                    new_dtp->dataloop_size = dtype_infos[total_op_count].dataloop_size;
                    new_dtp->dataloop_depth =
                        dtype_infos[total_op_count].dataloop_depth; 
                    new_dtp->eltype = dtype_infos[total_op_count].eltype;
                    /* set dataloop pointer */
                    new_dtp->dataloop = dataloops[total_op_count];
                    /* set datatype handle to be used in send/recv
                       below */
                    rma_op_infos[total_op_count].datatype = new_dtp->handle;

                    new_dtp->ub = dtype_infos[total_op_count].ub;
                    new_dtp->lb = dtype_infos[total_op_count].lb;
                    new_dtp->true_ub = dtype_infos[total_op_count].true_ub;
                    new_dtp->true_lb = dtype_infos[total_op_count].true_lb;
                    new_dtp->has_sticky_ub = dtype_infos[total_op_count].has_sticky_ub;
                    new_dtp->has_sticky_lb = dtype_infos[total_op_count].has_sticky_lb;
                    /* update pointers in dataloop */
                    ptrdiff = (MPI_Aint)((char *) (new_dtp->dataloop) - (char *)
                        (dtype_infos[total_op_count].dataloop));

                    MPID_Dataloop_update(new_dtp->dataloop, ptrdiff);
                }

                switch (rma_op_infos[total_op_count].type) {
                case MPIDI_RMA_PUT:
                    /* recv the put */
                    mpi_errno = NMPI_Recv((char *) win_ptr->base +
                                          win_ptr->disp_unit *
                                          rma_op_infos[total_op_count].disp,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    break;
                case MPIDI_RMA_GET:
                    /* send the get */
                    mpi_errno = NMPI_Send((char *) win_ptr->base +
                                          win_ptr->disp_unit *
                                          rma_op_infos[total_op_count].disp,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }
                    break;
                case MPIDI_RMA_ACCUMULATE:
                    /* recv the data into a temp buffer and perform
                       the reduction operation */
                    mpi_errno =
                        NMPI_Type_get_true_extent(rma_op_infos[total_op_count].datatype, 
                                                  &true_lb, &true_extent);  
                    if (mpi_errno) return mpi_errno;

                    MPID_Datatype_get_extent_macro(rma_op_infos[total_op_count].datatype, 
                                                   extent); 
                    tmp_buf = MPIU_Malloc(rma_op_infos[total_op_count].count * 
                                          (MPIR_MAX(extent,true_extent)));  
                    if (!tmp_buf) {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                        return mpi_errno;
                    }
                    /* adjust for potential negative lower bound in datatype */
                    tmp_buf = (void *)((char*)tmp_buf - true_lb);
                    
                    mpi_errno = NMPI_Recv(tmp_buf,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) {
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }

                    op = rma_op_infos[total_op_count].op;
                    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                        /* get the function by indexing into the op table */
                        uop = MPIR_Op_table[op%16 - 1];
                    }
                    else {
                        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                          "**opnotpredefined", "**opnotpredefined %d", op );
                        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                        return mpi_errno;
                    }

                    win_buf_addr = (char *) win_ptr->base +
                        win_ptr->disp_unit * rma_op_infos[total_op_count].disp;

                    if (rma_op_infos[total_op_count].datatype_kind ==
                        MPIDI_RMA_DATATYPE_BASIC) {
                        (*uop)(tmp_buf, win_buf_addr,
                               &(rma_op_infos[total_op_count].count),
                               &(rma_op_infos[total_op_count].datatype));
                    }
                    else { /* derived datatype */
                        MPID_Segment *segp;
                        DLOOP_VECTOR *dloop_vec;
                        MPI_Aint first, last;
			int vec_len;
                        MPI_Datatype type;
                        int type_size, count;

                        segp = MPID_Segment_alloc();
                        MPID_Segment_init(NULL,
                                          rma_op_infos[total_op_count].count, 
					  rma_op_infos[total_op_count].datatype,
					  segp, 0);
                        first = 0;
                        last  = SEGMENT_IGNORE_LAST;

                        vec_len = new_dtp->n_contig_blocks *
                            rma_op_infos[total_op_count].count + 1; 
                        /* +1 needed because Rob says  so */
                        dloop_vec = (DLOOP_VECTOR *)
                            MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
                        if (!dloop_vec) {
                            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, 
                                MPI_ERR_OTHER, "**nomem", 0 ); 
                            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                            return mpi_errno;
                        }

                        MPID_Segment_pack_vector(segp, first, &last,
                                                 dloop_vec, &vec_len);

                        type = new_dtp->eltype;
                        type_size = MPID_Datatype_get_basic_size(type);
                        for (i=0; i<vec_len; i++) {
                            count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
                            (*uop)((char *)tmp_buf + MPIU_PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                            (char *)win_buf_addr + MPIU_PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                                 &count, &type);
                        }

                        MPID_Segment_free(segp);
                        MPIU_Free(dloop_vec);
                    }

                    MPIU_Free((char*)tmp_buf + true_lb);
                    break;
                default:
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
                                                      "**opundefined_rma","**opundefined_rma %d",
						      rma_op_infos[total_op_count].type );
                    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
                    return mpi_errno;
                }
                tags[src]++;

                if (rma_op_infos[total_op_count].datatype_kind ==
                    MPIDI_RMA_DATATYPE_DERIVED) {
                    MPIU_Handle_obj_free(&MPID_Datatype_mem, new_dtp);
                    MPIU_Free(dataloops[total_op_count]);
                }
            }
        }

        mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
        if (mpi_errno) {
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);
            return mpi_errno;
        }

	MPIR_Nest_decr();

        MPIU_Free(tags);
        MPIU_Free(reqs);
        MPIU_Free(rma_op_infos);
        MPIU_Free(nops_to_proc);
        MPIU_Free(nops_from_proc);
        MPIU_Free(dtype_infos);
        for (i=0; i<total_op_count; i++)
            if (dataloops[i] != NULL) 
                MPIU_Free(dataloops[i]);
        MPIU_Free(dataloops);

        /* free MPIDI_RMA_ops_list */
        curr_ptr = MPIDI_RMA_ops_list;
        while (curr_ptr != NULL) {
            next_ptr = curr_ptr->next;
            MPIU_Free(curr_ptr);
            curr_ptr = next_ptr;
        }
        MPIDI_RMA_ops_list = NULL;

        if (assert & MPI_MODE_NOSUCCEED) 
            win_ptr->fence_cnt = 0;
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);

    return mpi_errno;
}
#endif
