/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int MPIDI_CH3I_Do_passive_target_rma(MPID_Win *win_ptr, int *wait_for_rma_done_pkt);
static int MPIDI_CH3I_Send_lock_put_or_acc(MPID_Win *win_ptr);
static int MPIDI_CH3I_Send_lock_get(MPID_Win *win_ptr);

#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_END_PT_EPOCH
        {
            mpi_errno = MPIDI_CH3_End_PT_epoch(dest, win_ptr);
        }
#       endif
    }
    else {
        int single_op_opt, type_size;
        MPIDI_RMA_ops *rma_op, *curr_op;
        MPID_Comm *comm_ptr;
        MPID_Request *req=NULL; 
        MPIDI_CH3_Pkt_t upkt;
        MPIDI_CH3_Pkt_lock_t *lock_pkt = &upkt.lock;
        MPIDI_VC_t * vc;
        int wait_for_rma_done_pkt = 0;
        
        if (dest == MPI_PROC_NULL) goto fn_exit;
        
        MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
        
        if (dest == comm_ptr->rank) {
            /* local lock. release the lock on the window, grant the next one
             * in the queue, and return. */
            mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
            if (mpi_errno != MPI_SUCCESS) goto fn_exit;
            mpi_errno = MPID_Progress_poke();
            goto fn_exit;
        }
        
        rma_op = win_ptr->rma_ops_list;
        
        if ( (rma_op == NULL) || (rma_op->type != MPIDI_RMA_LOCK) ) { 
            /* win_lock was not called. return error */
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**rmasync", 0 );
            goto fn_exit;
        }
        
        if (rma_op->target_rank != dest) {
            /* The target rank is different from the one passed to win_lock! */
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**winunlockrank", "**winunlockrank %d %d", dest, rma_op->target_rank);
            goto fn_exit;
        }
        
        if (rma_op->next == NULL) {
            /* only win_lock called, no put/get/acc. Do nothing and return. */
            MPIU_Free(rma_op);
            win_ptr->rma_ops_list = NULL;
            goto fn_exit;
        }
        
        single_op_opt = 0;
        
        if (rma_op->next->next == NULL) {
            /* Single put, get, or accumulate between the lock and unlock. If it
             * is of small size and predefined datatype at the target, we
             * do an optimization where the lock and the RMA operation are
             * sent in a single packet. Otherwise, we send a separate lock
             * request first. */
            
            curr_op = rma_op->next;
            
            MPID_Datatype_get_size_macro(curr_op->origin_datatype, type_size);
            
            if ( (HANDLE_GET_KIND(curr_op->target_datatype) == 
                  HANDLE_KIND_BUILTIN) &&
                 (type_size * curr_op->origin_count <= 
                  MPIDI_CH3_EAGER_MAX_MSG_SIZE) ) {
                single_op_opt = 1;
                /* Set the lock granted flag to 1 */
                win_ptr->lock_granted = 1;
                if (curr_op->type == MPIDI_RMA_GET) {
                    mpi_errno = MPIDI_CH3I_Send_lock_get(win_ptr);
                    wait_for_rma_done_pkt = 0;
                }
                else {
                    mpi_errno = MPIDI_CH3I_Send_lock_put_or_acc(win_ptr);
                    wait_for_rma_done_pkt = 1;
                }
            }
        }
        
        if (single_op_opt == 0) {
            
            /* Send a lock packet over to the target. wait for the lock_granted
             * reply. then do all the RMA ops. */ 
            
            MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
            lock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
            lock_pkt->source_win_handle = win_ptr->handle;
            lock_pkt->lock_type = rma_op->lock_type;
            
            MPIDI_Comm_get_vc(comm_ptr, dest, &vc);
            
            /* Set the lock granted flag to 0 */
            win_ptr->lock_granted = 0;
            
            mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_pkt, sizeof(*lock_pkt), &req);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "sending the rma message failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            /* release the request returned by iStartMsg */
            if (req != NULL)
            {
                MPID_Request_release(req);
            }
            
            /* After the target grants the lock, it sends a lock_granted
             * packet. This packet is received in ch3u_handle_recv_pkt.c.
             * The handler for the packet sets the win_ptr->lock_granted flag to 1. */
            
            /* poke the progress engine until lock_granted flag is set to 1 */
            if (win_ptr->lock_granted == 0)
            {
                MPID_Progress_state progress_state;
                
                MPID_Progress_start(&progress_state);
                while (win_ptr->lock_granted == 0)
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
            
            /* Now do all the RMA operations */
            mpi_errno = MPIDI_CH3I_Do_passive_target_rma(win_ptr, &wait_for_rma_done_pkt);
        }
        
        /* If the lock is a shared lock or we have done the single op
           optimization, we need to wait until the target informs us that
           all operations are done on the target. */ 
        if (wait_for_rma_done_pkt == 1) {
            /* wait until the "pt rma done" packet is received from the 
               target. This packet resets the win_ptr->lock_granted flag back to 
               0. */
            
            /* poke the progress engine until lock_granted flag is reset to 0 */
            if (win_ptr->lock_granted != 0)
            {
                MPID_Progress_state progress_state;
                
                MPID_Progress_start(&progress_state);
                while (win_ptr->lock_granted != 0)
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
        }
        else
            win_ptr->lock_granted = 0; 

    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Do_passive_target_rma
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Do_passive_target_rma(MPID_Win *win_ptr, int *wait_for_rma_done_pkt)
{
    int mpi_errno = MPI_SUCCESS, comm_size, done, i, nops;
    MPIDI_RMA_ops *curr_ptr, *next_ptr, **curr_ptr_ptr, *tmp_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests=NULL; /* array of requests */
    MPIDI_RMA_dtype_info *dtype_infos=NULL;
    void **dataloops=NULL;    /* to store dataloops for each datatype */
    MPI_Win source_win_handle, target_win_handle;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);

    if (win_ptr->rma_ops_list->lock_type == MPI_LOCK_EXCLUSIVE) {
        /* exclusive lock. no need to wait for rma done pkt at the end */
        *wait_for_rma_done_pkt = 0;
    }
    else {
        /* shared lock. check if any of the rma ops is a get. If so, move it 
           to the end of the list and do it last, in which case an rma done 
           pkt is not needed. If there is no get, rma done pkt is needed */

        /* First check whether the last operation is a get. Skip the first op, 
           which is a lock. */

        curr_ptr = win_ptr->rma_ops_list->next;
        while (curr_ptr->next != NULL) 
            curr_ptr = curr_ptr->next;
    
        if (curr_ptr->type == MPIDI_RMA_GET) {
            /* last operation is a get. no need to wait for rma done pkt */
            *wait_for_rma_done_pkt = 0;
        }
        else {
            /* go through the list and move the first get operation 
               (if there is one) to the end */
            
            curr_ptr = win_ptr->rma_ops_list->next;
            curr_ptr_ptr = &(win_ptr->rma_ops_list->next);
            
            *wait_for_rma_done_pkt = 1;
            
            while (curr_ptr != NULL) {
                if (curr_ptr->type == MPIDI_RMA_GET) {
                    *wait_for_rma_done_pkt = 0;
                    *curr_ptr_ptr = curr_ptr->next;
                    tmp_ptr = curr_ptr;
                    while (curr_ptr->next != NULL)
                        curr_ptr = curr_ptr->next;
                    curr_ptr->next = tmp_ptr;
                    tmp_ptr->next = NULL;
                    break;
                }
                else {
                    curr_ptr_ptr = &(curr_ptr->next);
                    curr_ptr = curr_ptr->next;
                }
            }
        }
    }

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );
    comm_size = comm_ptr->local_size;

    /* Ignore the first op in the list because it is a win_lock and do
       the rest */

    curr_ptr = win_ptr->rma_ops_list->next;
    nops = 0;
    while (curr_ptr != NULL) {
        nops++;
        curr_ptr = curr_ptr->next;
    }

    requests = (MPID_Request **) MPIU_Malloc(nops * sizeof(MPID_Request*));
    /* --BEGIN ERROR HANDLING-- */
    if (!requests)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    dtype_infos = (MPIDI_RMA_dtype_info *)
        MPIU_Malloc(nops*sizeof(MPIDI_RMA_dtype_info));
    /* --BEGIN ERROR HANDLING-- */
    if (!dtype_infos)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    dataloops = (void **) MPIU_Malloc(nops*sizeof(void*));
    /* allocate one extra for use when receiving data. see below */
    /* --BEGIN ERROR HANDLING-- */
    if (!dataloops)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    for (i=0; i<nops; i++)
    {
        dataloops[i] = NULL;
    }
    
    i = 0;
    curr_ptr = win_ptr->rma_ops_list->next;
    target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
    while (curr_ptr != NULL)
    {
        /* To indicate the last RMA operation, we pass the
           source_win_handle only on the last operation. Otherwise, 
           we pass MPI_WIN_NULL. */
        if (i == nops - 1)
            source_win_handle = win_ptr->handle;
        else 
            source_win_handle = MPI_WIN_NULL;
        
        switch (curr_ptr->type)
        {
        case (MPIDI_RMA_PUT):  /* same as accumulate */
        case (MPIDI_RMA_ACCUMULATE):
            win_ptr->pt_rma_puts_accs[curr_ptr->target_rank]++;
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
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "invalid RMA operation");
            goto fn_exit;
            /* --END ERROR HANDLING-- */
        }
        i++;
        curr_ptr = curr_ptr->next;
    }
    
    if (nops)
    {
	MPID_Progress_state progress_state;
	
	done = 1;
	MPID_Progress_start(&progress_state);
	while (nops)
	{
	    for (i=0; i<nops; i++)
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
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "making progress on the rma messages failed");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    done = 1;
	}
	MPID_Progress_end(&progress_state);
    } 
    
    MPIU_Free(requests);
    MPIU_Free(dtype_infos);
    for (i=0; i<nops; i++)
    {
        if (dataloops[i] != NULL)
        {
            MPIU_Free(dataloops[i]);
        }
    }
    MPIU_Free(dataloops);
    
    /* free MPIDI_RMA_ops_list */
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    win_ptr->rma_ops_list = NULL;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_put_or_acc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Send_lock_put_or_acc(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, lock_type, origin_dt_derived, iov_n, iovcnt;
    MPIDI_RMA_ops *rma_op;
    MPID_Request *request=NULL;
    MPIDI_VC_t * vc;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Comm *comm_ptr;
    MPID_Datatype *origin_dtp=NULL;
    int origin_type_size;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_put_unlock_t *lock_put_unlock_pkt = &upkt.lock_put_unlock;
    MPIDI_CH3_Pkt_lock_accum_unlock_t *lock_accum_unlock_pkt = &upkt.lock_accum_unlock;
        

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_PUT_OR_ACC);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_PUT_OR_ACC);

    lock_type = win_ptr->rma_ops_list->lock_type;

    rma_op = win_ptr->rma_ops_list->next;

    win_ptr->pt_rma_puts_accs[rma_op->target_rank]++;

    if (rma_op->type == MPIDI_RMA_PUT) {
        MPIDI_Pkt_init(lock_put_unlock_pkt, MPIDI_CH3_PKT_LOCK_PUT_UNLOCK);
        lock_put_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
        lock_put_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_put_unlock_pkt->lock_type = lock_type;
 
        lock_put_unlock_pkt->addr = 
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        
        lock_put_unlock_pkt->count = rma_op->target_count;
        lock_put_unlock_pkt->datatype = rma_op->target_datatype;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_put_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_put_unlock_pkt);
    }
    
    else if (rma_op->type == MPIDI_RMA_ACCUMULATE) {        
        MPIDI_Pkt_init(lock_accum_unlock_pkt, MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK);
        lock_accum_unlock_pkt->target_win_handle = 
            win_ptr->all_win_handles[rma_op->target_rank];
        lock_accum_unlock_pkt->source_win_handle = win_ptr->handle;
        lock_accum_unlock_pkt->lock_type = lock_type;

        lock_accum_unlock_pkt->addr = 
            (char *) win_ptr->base_addrs[rma_op->target_rank] +
            win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        
        lock_accum_unlock_pkt->count = rma_op->target_count;
        lock_accum_unlock_pkt->datatype = rma_op->target_datatype;
        lock_accum_unlock_pkt->op = rma_op->op;

        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_accum_unlock_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*lock_accum_unlock_pkt);
    }

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

    MPID_Datatype_get_size_macro(rma_op->origin_datatype, origin_type_size);

    if (!origin_dt_derived)
    {
	/* basic datatype on origin */

        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rma_op->origin_addr;
        iov[1].MPID_IOV_LEN = rma_op->origin_count * origin_type_size;
        iovcnt = 2;

        mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, iovcnt, &request);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
            goto fn_exit;
        }
	/* --END ERROR HANDLING-- */
    }
    else
    {
	/* derived datatype on origin */

        iovcnt = 1;

        request = MPID_Request_create();
        if (request == NULL) {
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
            /* --END ERROR HANDLING-- */
        }

        MPIU_Object_set_ref(request, 2);
        request->kind = MPID_REQUEST_SEND;
	    
        request->dev.datatype_ptr = origin_dtp;
        /* this will cause the datatype to be freed when the request
           is freed. */ 

        MPID_Segment_init(rma_op->origin_addr, rma_op->origin_count,
                          rma_op->origin_datatype,
                          &(request->dev.segment), 0);
        request->dev.segment_first = 0;
        request->dev.segment_size = rma_op->origin_count * origin_type_size;
	    
        iov_n = MPID_IOV_LIMIT - iovcnt;
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(request,
                                                     &iov[iovcnt],
                                                     &iov_n); 
        if (mpi_errno == MPI_SUCCESS)
        {
            iov_n += iovcnt;
            
            mpi_errno = MPIDI_CH3_iSendv(vc, request, iov, iov_n);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                MPID_Datatype_release(request->dev.datatype_ptr);
                MPIU_Object_set_ref(request, 0);
                MPIDI_CH3_Request_destroy(request);
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
                goto fn_exit;
            }
	    /* --END ERROR HANDLING-- */
        }
        /* --BEGIN ERROR HANDLING-- */
        else
        {
            MPID_Datatype_release(request->dev.datatype_ptr);
            MPIU_Object_set_ref(request, 0);
            MPIDI_CH3_Request_destroy(request);
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadsendiov", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
    }

    if (request != NULL) {
	if (*(request->cc_ptr) != 0)
        {
	    MPID_Progress_state progress_state;
	    
            MPID_Progress_start(&progress_state);
	    while (*(request->cc_ptr) != 0)
            {
                mpi_errno = MPID_Progress_wait(&progress_state);
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
		    MPID_Progress_end(&progress_state);
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", "**fail %s", "rma message operation failed");
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
            }
	    MPID_Progress_end(&progress_state);
        }
        
        mpi_errno = request->status.MPI_ERROR;
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "rma message operation failed");
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
                
        /* if origin datatype was a derived datatype, it will get 
           freed when the request gets freed. */ 
        MPID_Request_release(request);
    }

    /* free MPIDI_RMA_ops_list */
    MPIU_Free(win_ptr->rma_ops_list->next);
    MPIU_Free(win_ptr->rma_ops_list);
    win_ptr->rma_ops_list = NULL;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_PUT_OR_ACC);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Send_lock_get(MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, lock_type;
    MPIDI_RMA_ops *rma_op;
    MPID_Request *rreq=NULL, *sreq=NULL;
    MPIDI_VC_t * vc;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_Comm *comm_ptr;
    MPID_Datatype *dtp;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_get_unlock_t *lock_get_unlock_pkt = &upkt.lock_get_unlock;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GET);

    lock_type = win_ptr->rma_ops_list->lock_type;

    rma_op = win_ptr->rma_ops_list->next;

    /* create a request, store the origin buf, cnt, datatype in it,
       and pass a handle to it in the get packet. When the get
       response comes from the target, it will contain the request
       handle. */  
    rreq = MPID_Request_create();
    if (rreq == NULL) {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
        /* --END ERROR HANDLING-- */
    }

    MPIU_Object_set_ref(rreq, 2);

    rreq->dev.user_buf = rma_op->origin_addr;
    rreq->dev.user_count = rma_op->origin_count;
    rreq->dev.datatype = rma_op->origin_datatype;
    rreq->dev.target_win_handle = MPI_WIN_NULL;
    rreq->dev.source_win_handle = win_ptr->handle;
    if (HANDLE_GET_KIND(rreq->dev.datatype) != HANDLE_KIND_BUILTIN)
    {
        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        rreq->dev.datatype_ptr = dtp;
        /* this will cause the datatype to be freed when the
           request is freed. */  
    }

    MPIDI_Pkt_init(lock_get_unlock_pkt, MPIDI_CH3_PKT_LOCK_GET_UNLOCK);
    lock_get_unlock_pkt->target_win_handle = 
        win_ptr->all_win_handles[rma_op->target_rank];
    lock_get_unlock_pkt->source_win_handle = win_ptr->handle;
    lock_get_unlock_pkt->lock_type = lock_type;
 
    lock_get_unlock_pkt->addr = 
        (char *) win_ptr->base_addrs[rma_op->target_rank] +
        win_ptr->disp_units[rma_op->target_rank] * rma_op->target_disp;
        
    lock_get_unlock_pkt->count = rma_op->target_count;
    lock_get_unlock_pkt->datatype = rma_op->target_datatype;
    lock_get_unlock_pkt->request_handle = rreq->handle;

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) lock_get_unlock_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*lock_get_unlock_pkt);

    MPID_Comm_get_ptr(win_ptr->comm, comm_ptr);
    MPIDI_Comm_get_vc(comm_ptr, rma_op->target_rank, &vc);

    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_get_unlock_pkt, 
                                    sizeof(*lock_get_unlock_pkt), &sreq);
    if (mpi_errno != MPI_SUCCESS)
    {
     /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rmamsg", 0);
        goto fn_exit;
    /* --END ERROR HANDLING-- */
    }

    /* release the request returned by iStartMsg */
    if (sreq != NULL)
    {
        MPID_Request_release(sreq);
    }

    /* now wait for the data to arrive */
    if (*(rreq->cc_ptr) != 0)
    {
	MPID_Progress_state progress_state;
	
	MPID_Progress_start(&progress_state);
	while (*(rreq->cc_ptr) != 0)
        {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
		MPID_Progress_end(&progress_state);
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**fail", "**fail %s", "rma message operation failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }
	MPID_Progress_end(&progress_state);
    }
    
    mpi_errno = rreq->status.MPI_ERROR;
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**fail", "**fail %s", "rma message operation failed");
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
            
    /* if origin datatype was a derived datatype, it will get freed when the rreq gets freed. */ 
    MPID_Request_release(rreq);

    /* free MPIDI_RMA_ops_list */
    MPIU_Free(win_ptr->rma_ops_list->next);
    MPIU_Free(win_ptr->rma_ops_list);
    win_ptr->rma_ops_list = NULL;

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GET);
    return mpi_errno;
}
