/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int MPIDI_CH3I_Do_passive_target_rma(MPID_Win *win_ptr);

#undef FUNCNAME
#define FUNCNAME MPID_Win_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_RMA_ops *rma_op;
    MPID_Comm *comm_ptr;
    MPID_Request *req=NULL; 
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &upkt.lock;
    MPIDI_VC *vc;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

    if (dest == MPI_PROC_NULL) goto fn_exit;

    MPID_Comm_get_ptr( win_ptr->comm, comm_ptr );

    if (dest == comm_ptr->rank) {
        /* local lock. release the lock on the window, grant the next one
         * in the queue, and return. */
        mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
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

    /* Send a lock packet over to the target. wait for the lock_granted
     * reply. then do all the RMA ops. */ 
    
    lock_pkt->type = MPIDI_CH3_PKT_LOCK;
    lock_pkt->lock_type = rma_op->lock_type;
    lock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    lock_pkt->source_win_handle = win_ptr->handle;
    
    vc = comm_ptr->vcr[dest];
    
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
     * packet. This packet is received in ch3u_handle_receive_pkt.c.
     * The handler for the packet sets the win_ptr->lock_granted flag to 1. */
    
    /* poke the progress engine until lock_granted flag is set to 1 */
    while (win_ptr->lock_granted == 0)
    {
        MPID_Progress_start();
        
        if (win_ptr->lock_granted == 0)
        {
            mpi_errno = MPID_Progress_wait();
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "making progress on the rma messages failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
        }
        else
        {
            MPID_Progress_end();
            break;
        }
    }
    
    /* Now do all the RMA operations */
    mpi_errno = MPIDI_CH3I_Do_passive_target_rma(win_ptr);

    /* If the lock is a shared lock, we need to wait until the target informs 
       us that all operations are done on the target. */
    if (lock_pkt->lock_type == MPI_LOCK_SHARED) {
        /* wait until the "shared lock ops done" packet is received from the 
           target. This packet resets the win_ptr->lock_granted flag back to 
           0. */

        /* poke the progress engine until lock_granted flag is reset to 0 */
        while (win_ptr->lock_granted != 0)
        {
            MPID_Progress_start();
            
            if (win_ptr->lock_granted != 0)
            {
                mpi_errno = MPID_Progress_wait();
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "making progress on the rma messages failed");
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
            }
            else
            {
                MPID_Progress_end();
                break;
            }
        }
    }
    else
        win_ptr->lock_granted = 0; /* not really necessary, but we do it anyway */

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
}


static int MPIDI_CH3I_Do_passive_target_rma(MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, done, i, nops, target_win_handle;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPID_Comm *comm_ptr;
    MPID_Request **requests=NULL; /* array of requests */
    MPIDI_RMA_dtype_info *dtype_infos=NULL;
    void **dataloops=NULL;    /* to store dataloops for each datatype */

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_DO_PASSIVE_TARGET_RMA);

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
    while (curr_ptr != NULL)
    {
        /* To unlock the window at the target after the last RMA operation,
           we pass the target_win_handle only on the last operation. Otherwise, 
           we pass MPI_WIN_NULL. */
        if (i == nops - 1)
            target_win_handle = win_ptr->all_win_handles[curr_ptr->target_rank];
        else 
            target_win_handle = MPI_WIN_NULL;
        
        switch (curr_ptr->type)
        {
        case (MPIDI_RMA_PUT):  /* same as accumulate */
        case (MPIDI_RMA_ACCUMULATE):
            mpi_errno = MPIDI_CH3I_Send_rma_msg(curr_ptr, win_ptr,
                                                target_win_handle, &dtype_infos[i],
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
                                                target_win_handle, &dtype_infos[i],
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
    
    done = 1;
    while (nops)
    {
        MPID_Progress_start();
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
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "rma message operation failed");
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
        if (!done)
        {
            mpi_errno = MPID_Progress_wait();
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "making progress on the rma messages failed");
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            done = 1;
        }
        else
        {
            MPID_Progress_end();
            break;
        }
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




#ifdef USE_OLDSTUFF
int MPID_Win_unlock(int dest, MPID_Win *win_ptr)
{
    int mpi_errno=MPI_SUCCESS, nops_to_proc;
    int i, tag, req_cnt;
    MPIDI_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIDI_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        int datatype_kind;  /* basic or derived */
        MPI_Op op;
        int lock_type;
    } MPIDI_RMA_op_info;
    MPIDI_RMA_op_info *rma_op_infos;
    MPIDI_RMA_dtype_info *dtype_infos;
    void **dataloops;    /* to store dataloops for each datatype */
    MPI_Request *reqs;
    MPID_Datatype *dtp;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_UNLOCK);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_UNLOCK);

#ifdef MPICH_SINGLE_THREADED
    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**needthreads", 0 );
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
    return mpi_errno;
#endif

    MPIR_Nest_incr();

    comm = win_ptr->comm;

    /* First inform target process how many
       RMA ops from this process is it the target for.
       There could be ops destined for other processes in
       the rma_ops_list because there could be multiple MPI_Win_locks
       outstanding */

    nops_to_proc = 0;
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
        if (curr_ptr->target_rank == dest)
	{
	    nops_to_proc++;
	}
        curr_ptr = curr_ptr->next;
    }

    reqs = (MPI_Request *) MPIU_Malloc((4*nops_to_proc+1)*sizeof(MPI_Request));
    /* --BEGIN ERROR HANDLING-- */
    if (!reqs)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    tag = MPIDI_PASSIVE_TARGET_RMA_TAG;
    mpi_errno = NMPI_Isend(&nops_to_proc, 1, MPI_INT, dest,
                           tag, comm, reqs);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* For each RMA op, first send the type (put or get), target
       displ, count, datatype. Then issue an isend for a 
       put or irecv for a get. */
    
    rma_op_infos = (MPIDI_RMA_op_info *) 
        MPIU_Malloc((nops_to_proc+1) * sizeof(MPIDI_RMA_op_info));
    /* allocate one extra to avoid 0 size malloc */ 
    /* --BEGIN ERROR HANDLING-- */
    if (!rma_op_infos)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    dtype_infos = (MPIDI_RMA_dtype_info *)
        MPIU_Malloc((nops_to_proc+1)*sizeof(MPIDI_RMA_dtype_info));
    /* allocate one extra to avoid 0 size malloc */ 
    /* --BEGIN ERROR HANDLING-- */
    if (!dtype_infos)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    dataloops = (void **) MPIU_Malloc((nops_to_proc+1)*sizeof(void*));
    /* allocate one extra to avoid 0 size malloc */ 
    /* --BEGIN ERROR HANDLING-- */
    if (!dataloops)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    for (i=0; i<nops_to_proc; i++)
    {
        dataloops[i] = NULL;
    }

    i = 0;
    tag = 234;
    req_cnt = 1;
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
        rma_op_infos[i].type = curr_ptr->type;
        rma_op_infos[i].disp = curr_ptr->target_disp;
        rma_op_infos[i].count = curr_ptr->target_count;
        rma_op_infos[i].datatype = curr_ptr->target_datatype;
        rma_op_infos[i].op = curr_ptr->op;
        rma_op_infos[i].lock_type = curr_ptr->lock_type;

        if (rma_op_infos[i].type == MPIDI_RMA_LOCK)
	{
            rma_op_infos[i].datatype_kind = -1; /* undefined */
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
	    {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
            req_cnt++;
            tag++;
        }
        else if (HANDLE_GET_KIND(curr_ptr->target_datatype) ==
                 HANDLE_KIND_BUILTIN)
	{
            /* basic datatype. send only the rma_op_info struct */
            rma_op_infos[i].datatype_kind = MPIDI_RMA_DATATYPE_BASIC;
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
	    {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
            req_cnt++;
            tag++;
        }
        else
	{
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
	    /* --BEGIN ERROR HANDLING-- */
            if (!dataloops[i])
	    {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
            memcpy(dataloops[i], dtp->dataloop, dtp->dataloop_size);
            
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIDI_RMA_op_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
	    {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
            req_cnt++;
            tag++;
            
            /* send the datatype info */
            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&dtype_infos[i],
                                   sizeof(MPIDI_RMA_dtype_info), MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
	    {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
            req_cnt++;
            tag++;
            
            mpi_errno = NMPI_Isend(dataloops[i],
                                   dtp->dataloop_size, MPI_BYTE, 
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
	    {
                MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
                return mpi_errno;
            }
	    /* --END ERROR HANDLING-- */
            req_cnt++;
            tag++;
            
            /* release the target dataype */
            MPID_Datatype_release(dtp);
        }

        /* now send or recv the data */
        if ((curr_ptr->type == MPIDI_RMA_PUT) ||
            (curr_ptr->type == MPIDI_RMA_ACCUMULATE))
	{
            mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (HANDLE_GET_KIND(curr_ptr->origin_datatype) !=
                HANDLE_KIND_BUILTIN)
	    {
                MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
                MPID_Datatype_release(dtp);
            }
            req_cnt++;
            tag++;
        }
        else if (curr_ptr->type == MPIDI_RMA_GET)
	{
            mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                   curr_ptr->origin_count,
                                   curr_ptr->origin_datatype,
                                   dest, tag, comm,
                                   &reqs[req_cnt]); 
            if (HANDLE_GET_KIND(curr_ptr->origin_datatype) !=
                HANDLE_KIND_BUILTIN)
	    {
                MPID_Datatype_get_ptr(curr_ptr->origin_datatype, dtp);
                MPID_Datatype_release(dtp);
            }
            req_cnt++;
            tag++;
        }
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
            MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
        
        curr_ptr = curr_ptr->next;
        i++;
    }        

    mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* Passive target RMA must be complete at target when unlock
       returns. Therefore we need ack from target that it is done. */
    mpi_errno = NMPI_Recv(&i, 0, MPI_INT, dest,
                          MPIDI_PASSIVE_TARGET_DONE_TAG, comm,
                          MPI_STATUS_IGNORE);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    MPIR_Nest_decr();

    MPIU_Free(reqs);
    MPIU_Free(rma_op_infos);
    MPIU_Free(dtype_infos);
    for (i=0; i<nops_to_proc; i++)
    {
        if (dataloops[i] != NULL)
	{
            MPIU_Free(dataloops[i]);
	}
    }
    MPIU_Free(dataloops);

    /* free rma_ops_list */
    curr_ptr = win_ptr->rma_ops_list;
    while (curr_ptr != NULL)
    {
        next_ptr = curr_ptr->next;
        MPIU_Free(curr_ptr);
        curr_ptr = next_ptr;
    }
    win_ptr->rma_ops_list = NULL;
    
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_UNLOCK);

    return mpi_errno;
}
#endif
