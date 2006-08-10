/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef MPID_NEM_DONT_INLINE_FUNCTIONS
#undef MPID_NEM_INLINE_DECL
#define MPID_NEM_INLINE_DECL
#undef _MPID_NEM_INLINE_H /* ok to include again: we're including the non-inline functions in a .c file */
#else
#define MPID_NEM_INLINE_DECL extern inline
#endif

#ifndef _MPID_NEM_INLINE_H
#define _MPID_NEM_INLINE_H

#ifndef ENABLE_NO_SCHED_YIELD
#define MPID_NEM_POLLS_BEFORE_YIELD 1000
#endif

#include "my_papi_defs.h"

extern MPID_nem_cell_ptr_t MPID_nem_prefetched_cell;

/* MPID_nem_mpich2_send_header (void* buf, int size, MPIDI_VC_t *vc)
   same as above, but sends MPICH2 32 byte header */
#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_send_header
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_send_header (void* buf, int size, MPIDI_VC_t *vc, int *again)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t el;
    int my_rank;

#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
        goto return_again;
    }
#endif
    
    /*DO_PAPI (PAPI_reset (PAPI_EventSet)); */

    MPIU_Assert (size == sizeof(MPIDI_CH3_Pkt_t));

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (vc->ch.is_local)
    {
	MPID_nem_fbox_mpich2_t *pbox = vc->ch.fbox_out;
	int count = 10;
	u_int32_t *payload_32 = (u_int32_t *)pbox->cell.pkt.mpich2.payload;
	u_int32_t *buf_32 = (u_int32_t *)buf;

#ifdef BLOCKING_FBOX
	MPID_nem_waitforlock ((MPID_nem_fbox_common_ptr_t)pbox, 0, count);
#else /*BLOCKING_FBOX */
	if (MPID_nem_islocked ((MPID_nem_fbox_common_ptr_t)pbox, 0, count))
	    goto usequeue_l;
#endif /*BLOCKING_FBOX */
	{
	    pbox->cell.pkt.mpich2.source  = MPID_nem_mem_region.local_rank;
	    pbox->cell.pkt.mpich2.datalen = size;
	    pbox->cell.pkt.mpich2.seqno   = vc->ch.send_seqno++;

#ifdef ENABLED_CHECKPOINTING
	    pbox->cell.pkt.mpich2.datalen = size;
	    pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif /* ENABLED_CHECKPOINTING */
            MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2_HEAD);
            
	    payload_32[0] = buf_32[0];
	    payload_32[1] = buf_32[1];
	    payload_32[2] = buf_32[2];
	    payload_32[3] = buf_32[3];
	    payload_32[4] = buf_32[4];
	    payload_32[5] = buf_32[5];
	    payload_32[6] = buf_32[6];
	    payload_32[7] = buf_32[7];
	    if (sizeof(MPIDI_CH3_Pkt_t) == 40) /* This conditional should be optimized out */
	    {
		payload_32[8] = buf_32[8];
		payload_32[9] = buf_32[9];
	    }
	    
	    MPID_NEM_WRITE_BARRIER();
	    pbox->flag.value = 1;

	    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "--> Sent fbox ");
	    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (&pbox->cell));

	    goto return_success;
	}
    }
 usequeue_l:
#endif /*USE_FASTBOX */
    
#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el = MPID_nem_prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	    goto return_again;
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));
#else /* PREFETCH_CELL */
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	goto return_again;
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ , &el);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /* PREFETCH_CELL */

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = size;
    el->pkt.mpich2.seqno   = vc->ch.send_seqno++;
#ifdef ENABLED_CHECKPOINTING
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2_HEAD);
    
#if 1
    ((u_int32_t *)(el->pkt.mpich2.payload))[0] = ((u_int32_t *)buf)[0];
    ((u_int32_t *)(el->pkt.mpich2.payload))[1] = ((u_int32_t *)buf)[1];
    ((u_int32_t *)(el->pkt.mpich2.payload))[2] = ((u_int32_t *)buf)[2];
    ((u_int32_t *)(el->pkt.mpich2.payload))[3] = ((u_int32_t *)buf)[3];
    ((u_int32_t *)(el->pkt.mpich2.payload))[4] = ((u_int32_t *)buf)[4];
    ((u_int32_t *)(el->pkt.mpich2.payload))[5] = ((u_int32_t *)buf)[5];
    ((u_int32_t *)(el->pkt.mpich2.payload))[6] = ((u_int32_t *)buf)[6];
    ((u_int32_t *)(el->pkt.mpich2.payload))[7] = ((u_int32_t *)buf)[7];
    if (sizeof(MPIDI_CH3_Pkt_t) == 40) /* This conditional should be optimized out */
    {
	((u_int32_t *)(el->pkt.mpich2.payload))[8] = ((u_int32_t *)buf)[8];
	((u_int32_t *)(el->pkt.mpich2.payload))[9] = ((u_int32_t *)buf)[9];
    }
#else /*1 */
    MPID_NEM_MEMCPY (el->pkt.mpich2.payload, buf, size);
#endif /*1 */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11));

    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "--> Sent queue");
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (vc->ch.is_local)
    {
	MPID_nem_queue_enqueue (vc->ch.recv_queue, el);
	/*MPID_nem_rel_dump_queue( vc->ch.recv_queue ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, size);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues12));
    DO_PAPI (PAPI_reset (PAPI_EventSet));    

#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /*PREFETCH_CELL */

    /*DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14)); */

 return_success:
    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/*
  int MPID_nem_mpich2_sendv (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc);

  sends iov to vc
  Non-blocking
  if iov specifies more than MPID_NEM_MPICH2_DATA_LEN of data, the iov will be truncated, so that after MPID_nem_mpich2_sendv returns,
  iov will describe unsent data
  sets again to 1 if it can't get a free cell, 0 otherwise
*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_sendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_sendv (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc, int *again)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    int payload_len;    
    int my_rank;

    MPIU_Assert (n_iov > 0 && (*iov)->iov_len > 0);
    
#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
        goto return_again;
    }
#endif
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));

    my_rank = MPID_nem_mem_region.rank;
	
#ifdef PREFETCH_CELL
    el = MPID_nem_prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
            goto return_again;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL     */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
        goto return_again;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ , &el);
#endif /*PREFETCH_CELL     */

    payload_len = MPID_NEM_MPICH2_DATA_LEN;
    cell_buf    = (char *) el->pkt.mpich2.payload; /* cast away volatile */
    
    while (*n_iov && payload_len >= (*iov)->iov_len)
    {
	int _iov_len = (*iov)->iov_len;
	MPID_NEM_MEMCPY (cell_buf, (*iov)->iov_base, _iov_len);
	payload_len -= _iov_len;
	cell_buf += _iov_len;
	--(*n_iov);
	++(*iov);
    }
    
    if (*n_iov && payload_len > 0)
    {
	MPID_NEM_MEMCPY (cell_buf, (*iov)->iov_base, payload_len);
	(*iov)->iov_base += payload_len;
	(*iov)->iov_len -= payload_len;
 	payload_len = 0;
    }

    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = MPID_NEM_MPICH2_DATA_LEN - payload_len;
    el->pkt.mpich2.seqno   = vc->ch.send_seqno++;
#ifdef ENABLED_CHECKPOINTING
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2);

    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "--> Sent queue");
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    if(vc->ch.is_local)
    {
	MPID_nem_queue_enqueue (vc->ch.recv_queue, el);
	/*MPID_nem_rel_dump_queue( vc->ch.recv_queue ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, MPID_NEM_MPICH2_DATA_LEN - payload_len);
    }

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));

 return_success:
    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPID_nem_mpich2_sendv_header (struct iovec **iov, int *n_iov, int dest)
   same as above but first iov element is an MPICH2 32 byte header */
#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_send_header
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_sendv_header (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc, int *again)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    int payload_len;    
    int my_rank;

#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
        goto return_again;
    }
#endif
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPIU_Assert (*n_iov > 0 && (*iov)->iov_len == sizeof(MPIDI_CH3_Pkt_t));

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (vc->ch.is_local && (*n_iov == 2 && (*iov)[1].iov_len + sizeof(MPIDI_CH3_Pkt_t) <= MPID_NEM_FBOX_DATALEN))
    {
	MPID_nem_fbox_mpich2_t *pbox = vc->ch.fbox_out;
	int count = 10;
	u_int32_t *payload_32 = (u_int32_t *)(pbox->cell.pkt.mpich2.payload ) ;
	u_int32_t *buf_32 = (u_int32_t *)(*iov)->iov_base;

#ifdef BLOCKING_FBOX
	DO_PAPI3 (PAPI_reset (PAPI_EventSet));
	MPID_nem_waitforlock ((MPID_nem_fbox_common_ptr_t)pbox, 0, count);
#else /*BLOCKING_FBOX */
	if (MPID_nem_islocked ((MPID_nem_fbox_common_ptr_t)pbox, 0, count))
	    goto usequeue_l;
#endif /*BLOCKING_FBOX */
	{
	    pbox->cell.pkt.mpich2.source  = MPID_nem_mem_region.local_rank;
	    pbox->cell.pkt.mpich2.datalen = (*iov)[1].iov_len + sizeof(MPIDI_CH3_Pkt_t);
	    pbox->cell.pkt.mpich2.seqno   = vc->ch.send_seqno++;
#ifdef ENABLED_CHECKPOINTING
	    pbox->cell.pkt.mpich2.datalen = (*iov)[1].iov_len + sizeof(MPIDI_CH3_Pkt_t);
	    pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
            MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2_HEAD);
            
	    payload_32[0] = buf_32[0];
	    payload_32[1] = buf_32[1];
	    payload_32[2] = buf_32[2];
	    payload_32[3] = buf_32[3];
	    payload_32[4] = buf_32[4];
	    payload_32[5] = buf_32[5];
	    payload_32[6] = buf_32[6];
	    payload_32[7] = buf_32[7];
	    if (sizeof(MPIDI_CH3_Pkt_t) == 40) /* This conditional should be optimized out */
	    {
		payload_32[8] = buf_32[8];
		payload_32[9] = buf_32[9];
	    }
	    MPID_NEM_MEMCPY (((char *)(pbox->cell.pkt.mpich2.payload)) + sizeof(MPIDI_CH3_Pkt_t), (*iov)[1].iov_base,
			     (*iov)[1].iov_len);
	    MPID_NEM_WRITE_BARRIER();
	    pbox->flag.value = 1;
	    *n_iov = 0;

	    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "--> Sent fbox ");
	    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (&pbox->cell));

            goto return_success;
	}
    }
 usequeue_l:
    
#endif /*USE_FASTBOX */
	
#ifdef PREFETCH_CELL
    el = MPID_nem_prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
            goto return_again;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
        goto return_again;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
#endif /*PREFETCH_CELL */

    ((u_int32_t *)(el->pkt.mpich2.payload))[0] = ((u_int32_t *)(*iov)->iov_base)[0];
    ((u_int32_t *)(el->pkt.mpich2.payload))[1] = ((u_int32_t *)(*iov)->iov_base)[1];
    ((u_int32_t *)(el->pkt.mpich2.payload))[2] = ((u_int32_t *)(*iov)->iov_base)[2];
    ((u_int32_t *)(el->pkt.mpich2.payload))[3] = ((u_int32_t *)(*iov)->iov_base)[3];
    ((u_int32_t *)(el->pkt.mpich2.payload))[4] = ((u_int32_t *)(*iov)->iov_base)[4];
    ((u_int32_t *)(el->pkt.mpich2.payload))[5] = ((u_int32_t *)(*iov)->iov_base)[5];
    ((u_int32_t *)(el->pkt.mpich2.payload))[6] = ((u_int32_t *)(*iov)->iov_base)[6];
    ((u_int32_t *)(el->pkt.mpich2.payload))[7] = ((u_int32_t *)(*iov)->iov_base)[7];
    if (sizeof(MPIDI_CH3_Pkt_t) == 40) /* This conditional should be optimized out */
    {
	((u_int32_t *)(el->pkt.mpich2.payload))[8] = ((u_int32_t *)(*iov)->iov_base)[8];
	((u_int32_t *)(el->pkt.mpich2.payload))[9] = ((u_int32_t *)(*iov)->iov_base)[9];
    }

    cell_buf = (char *)(el->pkt.mpich2.payload) + sizeof(MPIDI_CH3_Pkt_t);
    ++(*iov);
    --(*n_iov);

    payload_len = MPID_NEM_MPICH2_DATA_LEN - sizeof(MPIDI_CH3_Pkt_t);
    while (*n_iov && payload_len >= (*iov)->iov_len)
    {
	int _iov_len = (*iov)->iov_len;
	MPID_NEM_MEMCPY (cell_buf, (*iov)->iov_base, _iov_len);
	payload_len -= _iov_len;
	cell_buf += _iov_len;
	--(*n_iov);
	++(*iov);
    }
    
    if (*n_iov && payload_len > 0)
    {
	MPID_NEM_MEMCPY (cell_buf, (*iov)->iov_base, payload_len);
	(*iov)->iov_base += payload_len;
	(*iov)->iov_len -= payload_len;
	payload_len = 0;
    }

    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = MPID_NEM_MPICH2_DATA_LEN - payload_len;
    el->pkt.mpich2.seqno   = vc->ch.send_seqno++;
#ifdef ENABLED_CHECKPOINTING
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2_HEAD);

    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "--> Sent queue");
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (el));

    if (vc->ch.is_local)
    {    
	MPID_nem_queue_enqueue (vc->ch.recv_queue, el);	
	/*MPID_nem_rel_dump_queue( vc->ch.recv_queue ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, MPID_NEM_MPICH2_DATA_LEN - payload_len);
    }

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &MPID_nem_prefetched_cell);
    else
	MPID_nem_prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));

 return_success:
    *again = 0;
    goto fn_exit;
 return_again:
    *again = 1;
    goto fn_exit;
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
  MPID_nem_mpich2_dequeue_fastbox (int local_rank)
  decrements usage count on fastbox for process with local rank local_rank and
  dequeues it from fbox queue if usage is 0.
  This function is called whenever a receive for a process on this node is matched.
  Fastboxes on fbox queue are polled regularly for incoming messages.
*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_dequeue_fastbox
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_dequeue_fastbox (int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_fboxq_elem_t *el;

    el = &MPID_nem_fboxq_elem_list[local_rank];    

    MPIU_ERR_CHKANDJUMP (!el->usage, mpi_errno, MPI_ERR_OTHER, "**intern");

    --el->usage;
    if (el->usage == 0)
    {
	if (el->prev == NULL)
	    MPID_nem_fboxq_head = el->next;
	else
	    el->prev->next = el->next;

	if (el->next == NULL)
	    MPID_nem_fboxq_tail = el->prev;
	else
	    el->next->prev = el->prev;

	if (el == MPID_nem_curr_fboxq_elem)
	{
	    if (el->next == NULL)
		MPID_nem_curr_fboxq_elem = MPID_nem_fboxq_head;
	    else
		MPID_nem_curr_fboxq_elem = el->next;
	}
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
  MPID_nem_mpich2_enqueue_fastbox (int local_rank)
  enqueues fastbox for process with local rank local_rank on fbox queue
  This function is called whenever a receive is posted for a process on this node.
  Fastboxes on fbox queue are polled regularly for incoming messages.
*/
#undef FUNCNAME
#define FUNCNAME MPID_nem_mpich2_dequeue_fastbox
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_enqueue_fastbox (int local_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_fboxq_elem_t *el;

    el = &MPID_nem_fboxq_elem_list[local_rank];
	
    if (el->usage)
    {
	++el->usage;
    }
    else
    {
	el->usage = 1;
	if (MPID_nem_fboxq_tail == NULL)
	{
	    el->prev = NULL;
	    MPID_nem_curr_fboxq_elem = MPID_nem_fboxq_head = el;
	}
	else
	{
	    el->prev = MPID_nem_fboxq_tail;
	    MPID_nem_fboxq_tail->next = el;
	}
	    
	el->next = NULL;
	MPID_nem_fboxq_tail = el;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/*
  MPID_nem_recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
  check whether the sequence number for the cell at the head of qhead is the one
  expected from the sender of that cell
  We only check these for processes in COMM_WORLD (i.e. the ones initially allocated)
*/
MPID_NEM_INLINE_DECL int
MPID_nem_recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
{
    MPID_nem_cell_ptr_t cell = MPID_NEM_REL_TO_ABS(qhead->my_head);
    int source = cell->pkt.mpich2.source;
    
    return (cell->pkt.mpich2.seqno == MPID_nem_recv_seqno[source]);
}

/*
  int MPID_nem_mpich2_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);

  non-blocking receive
  sets cell to the received cell, or NULL if there is nothing to receive. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich2_release_cell() once the packet has been copied out
*/
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int mpi_errno = MPI_SUCCESS;
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef ENABLED_CHECKPOINTING
    MPID_nem_ckpt_maybe_take_checkpoint();

    if (MPID_nem_ckpt_message_log)
    {
	MPID_nem_ckpt_replay_message (cell);
	MPIU_Assert ((*cell)->pkt.mpich2.seqno == MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source]);
	++MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source];
	*in_fbox = 0;
	goto fn_exit;
    }
#endif
    
#ifdef USE_FASTBOX
    poll_fboxes (cell, goto fbox_l);
#endif/* USE_FASTBOX     */

    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
    {
	MPID_nem_network_poll (MPID_NEM_POLL_IN);
    }

    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_recvQ) || !MPID_nem_recv_seqno_matches (MPID_nem_mem_region.my_recvQ))
    {
#ifdef USE_FASTBOX
	poll_all_fboxes (cell, goto fbox_l);
#endif/* USE_FASTBOX     */
	*cell = NULL;
	goto fn_exit;
    }
    
    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_recvQ, cell);

    ++MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source];
    *in_fbox = 0;

 fn_exit:
#ifdef ENABLED_CHECKPOINTING
    if ((*cell)->pkt.header.type == MPID_NEM_PKT_CKPT)
	MPID_nem_ckpt_got_marker (cell, in_fbox);
    else if (MPID_nem_ckpt_logging_messages)
	MPID_nem_ckpt_log_message (*cell);
#endif
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues6));
    
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, {
	if (*cell)
	{
	    MPIU_DBG_MSG_S (CH3_CHANNEL, VERBOSE, "<-- Recv %s", (*in_fbox) ? "fbox " : "queue");
	    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (*cell));
	}
    });
    
    return mpi_errno;

 fbox_l:
    *in_fbox = 1;
    goto fn_exit;

}

/*
  int MPID_nem_mpich2_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);

  blocking receive with timeout
  waits up to timeout iterations to receive a cell
  sets cell to the received cell, or NULL if there is nothing to receive. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich2_release_cell() once the packet has been copied out
*/
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout)
{
#ifdef USE_FASTBOX
    poll_fboxes (cell, goto fbox_l);
#endif/* USE_FASTBOX     */

    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
      {
	MPID_nem_network_poll (MPID_NEM_POLL_IN);
      }

    while ((--timeout > 0) && (MPID_nem_queue_empty (MPID_nem_mem_region.my_recvQ) || !MPID_nem_recv_seqno_matches (MPID_nem_mem_region.my_recvQ)))
    {
#ifdef USE_FASTBOX
	poll_all_fboxes (cell, goto fbox_l);
#endif/* USE_FASTBOX     */
	*cell = NULL;
	goto exit_l;
    }
    
    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_recvQ, cell);

    ++MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source];
    *in_fbox = 0;
 exit_l:
    
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, {
	if (*cell)
	{
	    MPIU_DBG_MSG_S (CH3_CHANNEL, VERBOSE, "<-- Recv %s", (*in_fbox) ? "fbox " : "queue");
	    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (*cell));
	}
    });
    
    return MPI_SUCCESS;

 fbox_l:
    *in_fbox = 1;
    goto exit_l;
}

/*
  int MPID_nem_mpich2_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);

  blocking receive
  waits until there is something to receive, then sets cell to the received cell. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich2_release_cell() once the packet has been copied out
*/
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
#ifndef ENABLE_NO_SCHED_YIELD
    int pollcount = 0;
#endif
    DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef ENABLED_CHECKPOINTING
    MPID_nem_ckpt_maybe_take_checkpoint();

 top_l:
    if (MPID_nem_ckpt_message_log)
    {
	MPID_nem_ckpt_replay_message (cell);
	MPIU_Assert ((*cell)->pkt.mpich2.seqno == MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source]);
	++MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source];
	*in_fbox = 0;
	return MPI_SUCCESS;
    }
#endif
    
    
#ifdef USE_FASTBOX
    poll_fboxes (cell, goto fbox_l);
#endif /*USE_FASTBOX */
   
    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
    {
	MPID_nem_network_poll (MPID_NEM_POLL_IN);
    }

    while (MPID_nem_queue_empty (MPID_nem_mem_region.my_recvQ) || !MPID_nem_recv_seqno_matches (MPID_nem_mem_region.my_recvQ))
    {
	DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef USE_FASTBOX	
	poll_all_fboxes (cell, goto fbox_l);
	poll_fboxes (cell, goto fbox_l);
#endif /*USE_FASTBOX */

	if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
	{
	    MPID_nem_network_poll (MPID_NEM_POLL_IN);
	}
#ifndef ENABLE_NO_SCHED_YIELD
	if (pollcount >= MPID_NEM_POLLS_BEFORE_YIELD)
	{
	    pollcount = 0;
	    sched_yield();
	}
	++pollcount;
#endif
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_recvQ, cell);

    ++MPID_nem_recv_seqno[(*cell)->pkt.mpich2.source];
    *in_fbox = 0;

 exit_l:    

#ifdef ENABLED_CHECKPOINTING
    if ((*cell)->pkt.header.type == MPID_NEM_PKT_CKPT)
    {
	MPID_nem_ckpt_got_marker (cell, in_fbox);
	goto top_l;
    }
    else if (MPID_nem_ckpt_logging_messages)
	MPID_nem_ckpt_log_message (*cell);
#endif

    DO_PAPI (PAPI_accum_var (PAPI_EventSet,PAPI_vvalues8));
    
    MPIU_DBG_MSG_S (CH3_CHANNEL, VERBOSE, "<-- Recv %s", (*in_fbox) ? "fbox " : "queue");
    MPIU_DBG_STMT (CH3_CHANNEL, VERBOSE, MPID_nem_dbg_dump_cell (*cell));

    return MPI_SUCCESS;

 fbox_l:
    *in_fbox = 1;
    goto exit_l;
}

/*
  int MPID_nem_mpich2_release_cell (MPID_nem_cell_ptr_t cell, MPIDI_VC_t *vc);

  releases the cell back to the subsystem to be used for subsequent receives
*/
MPID_NEM_INLINE_DECL int
MPID_nem_mpich2_release_cell (MPID_nem_cell_ptr_t cell, MPIDI_VC_t *vc)
{
    DO_PAPI (PAPI_reset (PAPI_EventSet));
#ifdef ENABLED_CHECKPOINTING
    if (cell->pkt.header.type == MPID_NEM_PKT_CKPT_REPLAY)
    {
	if (!MPID_nem_ckpt_message_log)
	    /* this is the last replayed message */
	    MPID_nem_ckpt_free_msg_log();
	return MPI_SUCCESS;
    }
#endif
    MPID_nem_queue_enqueue (vc->ch.free_queue, cell);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet,PAPI_vvalues9));
    return MPI_SUCCESS;
}

#endif /*_MPID_NEM_INLINE_H*/

