#ifdef MPID_NEM_DONT_INLINE_FUNCTIONS
#define MPID_NEM_INLINE_DECL
#undef _MPID_NEM_INLINE_H /* ok to include again: we're including the non-inline functions in a .c file */
#else
#undef MPID_NEM_INLINE_DECL
#define MPID_NEM_INLINE_DECL extern inline
#endif

#ifndef _MPID_NEM_INLINE_H
#define _MPID_NEM_INLINE_H

extern MPID_nem_cell_ptr_t prefetched_cell;

MPID_NEM_INLINE_DECL 
int MPID_nem_mpich2_send (void* buf, int size, int dest)
{
    MPID_nem_cell_ptr_t el;
    int my_rank;

#ifdef MPID_NEM_CKPT_ENABLED
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    assert (size <= MPID_NEM_MPICH2_DATA_LEN);
    my_rank = MPID_nem_mem_region.rank;
    
#ifdef PREFETCH_CELL
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	  return MPID_NEM_MPICH2_AGAIN;
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank])) 
    { 
      return MPID_NEM_MPICH2_AGAIN;
    }
    MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
#endif  /*PREFETCH_CELL      */
    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = dest;
    el->pkt.mpich2.datalen = size;
    el->pkt.mpich2.seqno   = send_seqno[dest]++;
#ifdef MPID_NEM_CKPT_ENABLED
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
    MPID_NEM_MEMCPY (el->pkt.mpich2.payload, buf, size);
    if(MPID_NEM_IS_LOCAL (dest))
    {
	MPID_nem_queue_enqueue( MPID_nem_mem_region.RecvQ[dest], el);
    }
    else
    {
      MPID_nem_net_module_send (dest, el, size);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues12));
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    
#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL    */
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_send_header (void* buf, int size, int dest)
{
    MPID_nem_cell_ptr_t el;
    int my_rank;

#ifdef MPID_NEM_CKPT_ENABLED
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    assert (size == MPID_NEM__MPICH2_HEADER_LEN);
    my_rank = MPID_nem_mem_region.rank;
#ifdef USE_FASTBOX
    if (MPID_NEM_IS_LOCAL (dest))
    {
	MPID_nem_fbox_mpich2_t *pbox = &MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[dest]]->mpich2;
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
	    //#ifdef BYPASS_PROGRESS
	    pbox->cell.pkt.mpich2.datalen = size; 
	    //#endif
	    pbox->cell.pkt.mpich2.seqno   = send_seqno[dest]++;
#if MPID_NEM__MPICH2_HEADER_LEN < 32
#error Cant handle case for MPICH2_HEADER_LEN < 32
#endif
#ifdef MPID_NEM_CKPT_ENABLED
	    pbox->cell.pkt.mpich2.datalen = size;
	    pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif /* MPID_NEM_CKPT_ENABLED */
	    payload_32[0] = buf_32[0];
	    payload_32[1] = buf_32[1];
	    payload_32[2] = buf_32[2];
	    payload_32[3] = buf_32[3];
	    payload_32[4] = buf_32[4];
	    payload_32[5] = buf_32[5];
	    payload_32[6] = buf_32[6];
	    payload_32[7] = buf_32[7];
#if MPID_NEM__MPICH2_HEADER_LEN == 40
	    payload_32[8] = buf_32[8];
    	    payload_32[9] = buf_32[9];
#endif /* MPID_NEM__MPICH2_HEADER_LEN == 40 */

#if MPID_NEM__MPICH2_HEADER_LEN > 40
#error Cant handle case for MPICH2_HEADER_LEN >40
#endif /* MPID_NEM__MPICH2_HEADER_LEN > 40 */
	    MPID_NEM_WRITE_BARRIER();
	    pbox->flag.value = 1;
	
	    return MPID_NEM_MPICH2_SUCCESS;
	}
    }
 usequeue_l:
#endif /*USE_FASTBOX */
    
#ifdef PREFETCH_CELL
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	    return MPID_NEM_MPICH2_AGAIN;
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
#else /* PREFETCH_CELL */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
    {
      return MPID_NEM_MPICH2_AGAIN;
    }
    MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank] , &el);
#endif /* PREFETCH_CELL */
    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = dest;
    el->pkt.mpich2.datalen = size;
    el->pkt.mpich2.seqno   = send_seqno[dest]++;
#ifdef MPID_NEM_CKPT_ENABLED
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
#if 1
#if MPID_NEM__MPICH2_HEADER_LEN < 32
#error Cant handle case for MPICH2_HEADER_LEN < 32
#endif
    ((u_int32_t *)(el->pkt.mpich2.payload))[0] = ((u_int32_t *)buf)[0];
    ((u_int32_t *)(el->pkt.mpich2.payload))[1] = ((u_int32_t *)buf)[1];
    ((u_int32_t *)(el->pkt.mpich2.payload))[2] = ((u_int32_t *)buf)[2];
    ((u_int32_t *)(el->pkt.mpich2.payload))[3] = ((u_int32_t *)buf)[3];
    ((u_int32_t *)(el->pkt.mpich2.payload))[4] = ((u_int32_t *)buf)[4];
    ((u_int32_t *)(el->pkt.mpich2.payload))[5] = ((u_int32_t *)buf)[5];
    ((u_int32_t *)(el->pkt.mpich2.payload))[6] = ((u_int32_t *)buf)[6];
    ((u_int32_t *)(el->pkt.mpich2.payload))[7] = ((u_int32_t *)buf)[7];
#if MPID_NEM__MPICH2_HEADER_LEN == 40
    ((u_int32_t *)(el->pkt.mpich2.payload))[8] = ((u_int32_t *)buf)[8];
    ((u_int32_t *)(el->pkt.mpich2.payload))[9] = ((u_int32_t *)buf)[9];
#endif
#if MPID_NEM__MPICH2_HEADER_LEN > 40
#error Cant handle case for MPICH2_HEADER_LEN >40
#endif
#else /*1 */
    MPID_NEM_MEMCPY (el->pkt.mpich2.payload, buf, size);
#endif /*1 */
    if (MPID_NEM_IS_LOCAL (dest))
    {
	MPID_nem_queue_enqueue( MPID_nem_mem_region.RecvQ[dest], el);
    }
    else
    {
       MPID_nem_net_module_send (dest, el, size);
    }
#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_sendv (struct iovec **iov, int *n_iov, int dest)
{
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    int payload_len;    
    int my_rank;

#ifdef MPID_NEM_CKPT_ENABLED
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    my_rank = MPID_nem_mem_region.rank;	
#ifdef PREFETCH_CELL
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	{
	    return MPID_NEM_MPICH2_AGAIN;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
#else /*PREFETCH_CELL     */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
    {
	return MPID_NEM_MPICH2_AGAIN;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank] , &el);
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
    el->pkt.mpich2.dest    = dest;
    el->pkt.mpich2.datalen = MPID_NEM_MPICH2_DATA_LEN - payload_len;
    el->pkt.mpich2.seqno   = send_seqno[dest]++;
#ifdef MPID_NEM_CKPT_ENABLED
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif

    if(MPID_NEM_IS_LOCAL (dest))
    {
	MPID_nem_queue_enqueue (MPID_nem_mem_region.RecvQ[dest], el);
    }
    else
    {
       MPID_nem_net_module_send (dest, el, MPID_NEM_MPICH2_DATA_LEN - payload_len);
    }

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL 
int MPID_nem_mpich2_sendv_header (struct iovec **iov, int *n_iov, int dest)
{
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    int payload_len;    
    int my_rank;

#ifdef MPID_NEM_CKPT_ENABLED
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    assert (*n_iov >= 1 && (*iov)->iov_len == MPID_NEM__MPICH2_HEADER_LEN);

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (MPID_NEM_IS_LOCAL (dest) && (*n_iov == 2 && (*iov)[1].iov_len + MPID_NEM__MPICH2_HEADER_LEN <= MPID_NEM_FBOX_DATALEN))
    {
	MPID_nem_fbox_mpich2_t *pbox = &MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[dest]]->mpich2;
	int count = 10;
	u_int32_t *payload_32 = (u_int32_t *)(pbox->cell.pkt.mpich2.payload ) ;
	u_int32_t *buf_32 = (u_int32_t *)(*iov)->iov_base;

#ifdef BLOCKING_FBOX
	MPID_nem_waitforlock ((MPID_nem_fbox_common_ptr_t)pbox, 0, count);
#else /*BLOCKING_FBOX */
	if (MPID_nem_islocked ((MPID_nem_fbox_common_ptr_t)pbox, 0, count))
	    goto usequeue_l;
#endif /*BLOCKING_FBOX */
	{
	    pbox->cell.pkt.mpich2.source  = MPID_nem_mem_region.local_rank;
	    //#ifdef BYPASS_PROGRESS
	    pbox->cell.pkt.mpich2.datalen = (*iov)[1].iov_len + MPID_NEM__MPICH2_HEADER_LEN; 
	    //#endif
	    pbox->cell.pkt.mpich2.seqno   = send_seqno[dest]++;
#if MPID_NEM__MPICH2_HEADER_LEN < 32
#error Cant handle case for MPICH2_HEADER_LEN < 32
#endif
#ifdef MPID_NEM_CKPT_ENABLED
	    pbox->cell.pkt.mpich2.datalen = (*iov)[1].iov_len + MPID_NEM__MPICH2_HEADER_LEN;
	    pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
	    payload_32[0] = buf_32[0];
	    payload_32[1] = buf_32[1];
	    payload_32[2] = buf_32[2];
	    payload_32[3] = buf_32[3];
	    payload_32[4] = buf_32[4];
	    payload_32[5] = buf_32[5];
	    payload_32[6] = buf_32[6];
	    payload_32[7] = buf_32[7];
#if MPID_NEM__MPICH2_HEADER_LEN == 40
	    payload_32[8] = buf_32[8];
    	    payload_32[9] = buf_32[9];
#endif
#if MPID_NEM__MPICH2_HEADER_LEN > 40
#error Cant handle case for MPICH2_HEADER_LEN >40
#endif
	    MPID_NEM_MEMCPY (((char *)(pbox->cell.pkt.mpich2.payload)) + MPID_NEM__MPICH2_HEADER_LEN, (*iov)[1].iov_base, (*iov)[1].iov_len);
	    MPID_NEM_WRITE_BARRIER();
	    pbox->flag.value = 1;
	    *n_iov = 0;

	    return MPID_NEM_MPICH2_SUCCESS;
	}
    }
 usequeue_l:
    
#endif /*USE_FASTBOX */
	
#ifdef PREFETCH_CELL
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	{
	    return MPID_NEM_MPICH2_AGAIN;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
    {
	return MPID_NEM_MPICH2_AGAIN;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
#endif /*PREFETCH_CELL */

#if MPID_NEM__MPICH2_HEADER_LEN < 32
#error Cant handle case for MPICH2_HEADER_LEN < 32
#endif
    ((u_int32_t *)(el->pkt.mpich2.payload))[0] = ((u_int32_t *)(*iov)->iov_base)[0];
    ((u_int32_t *)(el->pkt.mpich2.payload))[1] = ((u_int32_t *)(*iov)->iov_base)[1];
    ((u_int32_t *)(el->pkt.mpich2.payload))[2] = ((u_int32_t *)(*iov)->iov_base)[2];
    ((u_int32_t *)(el->pkt.mpich2.payload))[3] = ((u_int32_t *)(*iov)->iov_base)[3];
    ((u_int32_t *)(el->pkt.mpich2.payload))[4] = ((u_int32_t *)(*iov)->iov_base)[4];
    ((u_int32_t *)(el->pkt.mpich2.payload))[5] = ((u_int32_t *)(*iov)->iov_base)[5];
    ((u_int32_t *)(el->pkt.mpich2.payload))[6] = ((u_int32_t *)(*iov)->iov_base)[6];
    ((u_int32_t *)(el->pkt.mpich2.payload))[7] = ((u_int32_t *)(*iov)->iov_base)[7];
#if MPID_NEM__MPICH2_HEADER_LEN == 40
    ((u_int32_t *)(el->pkt.mpich2.payload))[8] = ((u_int32_t *)(*iov)->iov_base)[8];
    ((u_int32_t *)(el->pkt.mpich2.payload))[9] = ((u_int32_t *)(*iov)->iov_base)[9];
#endif
#if MPID_NEM__MPICH2_HEADER_LEN > 40
#error Cant handle case for MPICH2_HEADER_LEN >40
#endif

    cell_buf = (char *)(el->pkt.mpich2.payload) + MPID_NEM__MPICH2_HEADER_LEN;
    ++(*iov);
    --(*n_iov);

    payload_len = MPID_NEM_MPICH2_DATA_LEN - MPID_NEM__MPICH2_HEADER_LEN;
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
    el->pkt.mpich2.dest    = dest;
    el->pkt.mpich2.datalen = MPID_NEM_MPICH2_DATA_LEN - payload_len;
    el->pkt.mpich2.seqno   = send_seqno[dest]++;
#ifdef MPID_NEM_CKPT_ENABLED
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif

    if (MPID_NEM_IS_LOCAL (dest))
    {    
	MPID_nem_queue_enqueue (MPID_nem_mem_region.RecvQ[dest], el);	
    }
    else
    {
       MPID_nem_net_module_send (dest, el, MPID_NEM_MPICH2_DATA_LEN - payload_len);
    }
    
#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_dequeue_fastbox (int local_rank)
{
    int errno = MPID_NEM_MPICH2_SUCCESS;
    MPID_nem_fboxq_elem_t *el;

    el = &fboxq_elem_list[local_rank];

    if (!el->usage)
    {
	errno = MPID_NEM_MPICH2_FAILURE;
	goto exit_l;
    }

    --el->usage;
    if (el->usage == 0)
    {
	if (el->prev == NULL)
	    fboxq_head = el->next;
	else
	    el->prev->next = el->next;

	if (el->next == NULL)
	    fboxq_tail = el->prev;
	else
	    el->next->prev = el->prev;

	if (el == curr_fboxq_elem)
	{
	    if (el->next == NULL)
		curr_fboxq_elem = fboxq_head;
	    else
		curr_fboxq_elem = el->next;
	}
     }
    
 exit_l:
    return errno;
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_enqueue_fastbox (int local_rank)
{
    int errno = MPID_NEM_MPICH2_SUCCESS;
    MPID_nem_fboxq_elem_t *el;

    el = &fboxq_elem_list[local_rank];
	
    if (el->usage)
    {
	++el->usage;
    }
    else
    {
	el->usage = 1;
	if (fboxq_tail == NULL)
	{
	    el->prev = NULL;
	    curr_fboxq_elem = fboxq_head = el;
	}
	else
	{
	    el->prev = fboxq_tail;
	    fboxq_tail->next = el;
	}
	    
	el->next = NULL;
	fboxq_tail = el;
    }
    
    return errno;
}

MPID_NEM_INLINE_DECL 
int recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
{
    MPID_nem_cell_ptr_t cell = MPID_NEM_REL_TO_ABS(qhead->my_head);
    int source = cell->pkt.mpich2.source;
    
    return (cell->pkt.mpich2.seqno == recv_seqno[source]);
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int my_rank = MPID_nem_mem_region.rank;
#ifdef MPID_NEM_CKPT_ENABLED
    MPID_nem_ckpt_maybe_take_checkpoint();

    if (MPID_nem_ckpt_message_log)
    {
	MPID_nem_ckpt_replay_message (cell);
	assert ((*cell)->pkt.mpich2.seqno == recv_seqno[(*cell)->pkt.mpich2.source]);
	++recv_seqno[(*cell)->pkt.mpich2.source];
	*in_fbox = 0;
	return MPID_NEM_MPICH2_SUCCESS;
    }
#endif
    
#ifdef USE_FASTBOX
    poll_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif/* USE_FASTBOX     */

    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
      {
	MPID_nem_network_poll (MPID_NEM_POLL_IN);
      }

    if (MPID_nem_queue_empty (MPID_nem_mem_region.RecvQ[my_rank]) || !recv_seqno_matches (MPID_nem_mem_region.RecvQ[my_rank]))
    {
#ifdef USE_FASTBOX
	poll_all_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif/* USE_FASTBOX     */
	*cell = NULL;
	goto exit_l;
    }
    
    MPID_nem_queue_dequeue (MPID_nem_mem_region.RecvQ[my_rank], cell);
    ++recv_seqno[(*cell)->pkt.mpich2.source];
    *in_fbox = 0;
 exit_l:
#ifdef MPID_NEM_CKPT_ENABLED
    if ((*cell)->pkt.header.type == MPID_NEM_PKT_CKPT)
	MPID_nem_ckpt_got_marker (cell, in_fbox);
    else if (MPID_nem_ckpt_logging_messages)
	MPID_nem_ckpt_log_message (*cell);
#endif
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL 
int MPID_nem_mpich2_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout)
{
    int my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    poll_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif/* USE_FASTBOX     */

    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
      {
	MPID_nem_network_poll (MPID_NEM_POLL_IN);
      }

    while ((--timeout > 0) && (MPID_nem_queue_empty (MPID_nem_mem_region.RecvQ[my_rank]) || !recv_seqno_matches (MPID_nem_mem_region.RecvQ[my_rank])))
    {
#ifdef USE_FASTBOX
	poll_all_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif/* USE_FASTBOX     */
	*cell = NULL;
	goto exit_l;
    }
    
    MPID_nem_queue_dequeue (MPID_nem_mem_region.RecvQ[my_rank], cell);

    ++recv_seqno[(*cell)->pkt.mpich2.source];
    *in_fbox = 0;
 exit_l:
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int my_rank = MPID_nem_mem_region.rank;

#ifdef MPID_NEM_CKPT_ENABLED
    MPID_nem_ckpt_maybe_take_checkpoint();

 top_l:
    if (MPID_nem_ckpt_message_log)
    {
	MPID_nem_ckpt_replay_message (cell);
	assert ((*cell)->pkt.mpich2.seqno == recv_seqno[(*cell)->pkt.mpich2.source]);
	++recv_seqno[(*cell)->pkt.mpich2.source];
	*in_fbox = 0;
	return MPID_NEM_MPICH2_SUCCESS;
    }
#endif
    

#ifdef USE_FASTBOX
    poll_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif /*USE_FASTBOX */
   
    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
    {
	MPID_nem_network_poll (MPID_NEM_POLL_IN);
    }

    while (MPID_nem_queue_empty (MPID_nem_mem_region.RecvQ[my_rank]) || !recv_seqno_matches (MPID_nem_mem_region.RecvQ[my_rank]))
    {
#ifdef USE_FASTBOX	
	poll_all_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
	poll_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif /*USE_FASTBOX */

	if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
	{
	    MPID_nem_network_poll (MPID_NEM_POLL_IN);
	}
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.RecvQ[my_rank], cell);
    ++recv_seqno[(*cell)->pkt.mpich2.source];
    *in_fbox = 0;    

 exit_l:
#ifdef MPID_NEM_CKPT_ENABLED
    if ((*cell)->pkt.header.type == MPID_NEM_PKT_CKPT)
    {
	MPID_nem_ckpt_got_marker (cell, in_fbox);
	goto top_l;
    }
    else if (MPID_nem_ckpt_logging_messages)
	MPID_nem_ckpt_log_message (*cell);
#endif
    return MPID_NEM_MPICH2_SUCCESS;
}

MPID_NEM_INLINE_DECL
int MPID_nem_mpich2_release_cell (MPID_nem_cell_ptr_t cell)
{
    int source = cell->pkt.mpich2.source;

#ifdef MPID_NEM_CKPT_ENABLED
    if (cell->pkt.header.type == MPID_NEM_PKT_CKPT_REPLAY)
    {
	if (!MPID_nem_ckpt_message_log)
	    /* this is the last replayed message */
	    MPID_nem_ckpt_free_msg_log();
	return MPID_NEM_MPICH2_SUCCESS;
    }
#endif
    
    /*MPID_nem_cell_init(cell); */

    if (MPID_NEM_IS_LOCAL (source))
    {
	MPID_nem_queue_enqueue (MPID_nem_mem_region.FreeQ[source], cell);
    }
    else
    {  
	MPID_nem_queue_enqueue (MPID_nem_mem_region.FreeQ[source], cell);
    }
    return MPID_NEM_MPICH2_SUCCESS;
}

#endif //_MPID_NEM_INLINE_H

