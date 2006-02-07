#include "mpid_nem.h"
#include "mpid_nem_fbox.h"
#include "mpid_nem_nets.h"

/* #define BLOCKING_FBOX */

#define DO_PAPI(x)  /*x */
#define DO_PAPI2(x) /*x */
#define DO_PAPI3(x) /*x */

MPID_nem_fboxq_elem_t *fboxq_head;
MPID_nem_fboxq_elem_t *fboxq_tail;
MPID_nem_fboxq_elem_t *fboxq_elem_list;
MPID_nem_fboxq_elem_t *fboxq_elem_list_last;
MPID_nem_fboxq_elem_t *curr_fboxq_elem;
MPID_nem_fboxq_elem_t *curr_fbox_all_poll;

extern int MPID_nem_ckpt_logging_messages; /* are we in logging-message-mode? */
extern int MPID_nem_ckpt_sending_markers; /* are we in the process of sending markers? */
extern struct cli_message_log_total *MPID_nem_ckpt_message_log; /* are we replaying messages? */

/*static int gm_poll_count = 0; */
/*#define GM_POLL_FREQ (!(gm_poll_count++ % 100)) */
/*#define GM_POLL_FREQ (gm_poll_count == gm_poll_count) */
/*#define GM_POLL_FREQ (gm_poll_count != gm_poll_count) */

MPID_nem_cell_ptr_t prefetched_cell;

unsigned short *send_seqno;
unsigned short *recv_seqno;

#ifndef ENABLE_NO_SCHED_YIELD
#define POLLS_BEFORE_YIELD 1000
#endif

/* here we include the non-inlined versions of the files in mpid_nem_inline.h */
#define MPID_NEM_DONT_INLINE_FUNCTIONS 1
#include <mpid_nem_inline.h>


void
MPID_nem_mpich2_init (int ckpt_restart)
{
    int i;

    /*     printf ("sizeof (MPID_nem_cell_t) == %u\n", sizeof (MPID_nem_cell_t)); */
    /*     printf ("&MPID_nem_mem_region.mailboxes.in[0]->mpich2 = %p\n", &MPID_nem_mem_region.mailboxes.in[0]->mpich2); */
    /*     printf ("&MPID_nem_mem_region.mailboxes.in[1]->mpich2 = %p\n", &MPID_nem_mem_region.mailboxes.in[1]->mpich2); */
    /*     printf ("sizeof (MPID_nem_fastbox_t) = %u\n", sizeof (MPID_nem_fastbox_t)); */
    /*     printf ("sizeof (MPID_nem_mem_region.mailboxes.in[1]->mpich2) = %u\n", sizeof (MPID_nem_mem_region.mailboxes.in[1]->mpich2)); */
    /*     printf ("OFFSETPF (MPID_nem_fbox_mpich2_t, cell) = %u\n", MPID_NEM_OFFSETOF(MPID_nem_fbox_mpich2_t, cell)); */

    prefetched_cell = NULL;
    
    if (!ckpt_restart)
    {
	send_seqno = MALLOC (sizeof(*send_seqno) * MPID_nem_mem_region.num_procs);
	recv_seqno = MALLOC (sizeof(*recv_seqno) * MPID_nem_mem_region.num_procs);
	if (!send_seqno || !recv_seqno)
	    FATAL_ERROR ("malloc failed");

	for (i = 0; i < MPID_nem_mem_region.num_procs; ++i)
	{
	    send_seqno[i] = 0;
	    recv_seqno[i] = 0;
	}
    
	/* set up fbox queue */
	fboxq_elem_list = MALLOC (MPID_nem_mem_region.num_local * sizeof(MPID_nem_fboxq_elem_t));
	if (!fboxq_elem_list)
	    FATAL_ERROR ("malloc failed");
    
	for (i = 0; i < MPID_nem_mem_region.num_local; ++i)
	{
	    fboxq_elem_list[i].usage = 0;
	    fboxq_elem_list[i].prev = NULL;
	    fboxq_elem_list[i].next = NULL;
	    fboxq_elem_list[i].grank = MPID_nem_mem_region.local_procs[i];
	    fboxq_elem_list[i].fbox = &MPID_nem_mem_region.mailboxes.in[i]->mpich2;
	}
	
	fboxq_head = NULL;
	fboxq_tail = NULL;
	curr_fboxq_elem = NULL;
	curr_fbox_all_poll = &fboxq_elem_list[0];
	fboxq_elem_list_last = &fboxq_elem_list[MPID_nem_mem_region.num_local - 1];
    }
    else
    {
	for (i = 0; i < MPID_nem_mem_region.num_local; ++i)
	{
	    assert (fboxq_elem_list[i].grank == MPID_nem_mem_region.local_procs[i]);
	    fboxq_elem_list[i].fbox = &MPID_nem_mem_region.mailboxes.in[i]->mpich2;
	}

	fboxq_head = NULL;
	fboxq_tail = NULL;
	curr_fboxq_elem = NULL;
	curr_fbox_all_poll = &fboxq_elem_list[0];
	fboxq_elem_list_last = &fboxq_elem_list[MPID_nem_mem_region.num_local - 1];
    }
}

/*
  int MPID_nem_mpich2_send_ckpt_marker (unsigned short wave, MPIDI_VC_t *vc);

  sends checkpoint marker with wave number wave to vc
  Non-blocking
  returns MPID_NEM_MPICH2_AGAIN if it can't get a free cell
*/
int
MPID_nem_mpich2_send_ckpt_marker (unsigned short wave, MPIDI_VC_t *vc)
{
#ifdef ENABLED_CHECKPOINTING
    MPID_nem_cell_ptr_t el;
    int my_rank;

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

    el->pkt.ckpt.source  = my_rank;
    el->pkt.ckpt.dest    = vc->lpid;
    el->pkt.ckpt.datalen = sizeof(el->pkt.ckpt.wave); /* FIXME: we need a way to handle packet types w/ different sizes */
    el->pkt.ckpt.seqno   = send_seqno[vc->lpid]++;
    el->pkt.ckpt.type = MPID_NEM_PKT_CKPT;
    el->pkt.ckpt.wave = wave;

    if(MPID_NEM_IS_LOCAL (vc->lpid))
    {
	MPID_nem_queue_enqueue( MPID_nem_mem_region.RecvQ[vc->lpid], el);
	/*MPID_nem_rel_dump_queue( MPID_nem_mem_region.RecvQ[vc->lpid] ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, el->pkt.ckpt.datalen);
    }


#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
#endif /*ENABLED_CHECKPOINTING */
    return MPID_NEM_MPICH2_SUCCESS;
}

#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS
/*
  int MPID_nem_mpich2_send (void* buf, int size, MPIDI_VC_t *vc);

  sends buf to vc
  Non-blocking
  size must not be greater than MPID_NEM_MPICH2_DATA_LEN
  returns MPID_NEM_MPICH2_AGAIN if it can't get a free cell
*/
int
MPID_nem_mpich2_send (void* buf, int size, MPIDI_VC_t *vc)
{
    MPID_nem_cell_ptr_t el;
    int my_rank;

#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif

    /*DO_PAPI (PAPI_reset (PAPI_EventSet)); */

    assert (size <= MPID_NEM_MPICH2_DATA_LEN);
    my_rank = MPID_nem_mem_region.rank;
    
#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	    return MPID_NEM_MPICH2_AGAIN;
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));
#else /*PREFETCH_CELL    */
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank])) 
    { 
	return MPID_NEM_MPICH2_AGAIN;
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif  /*PREFETCH_CELL      */

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = size;
    el->pkt.mpich2.seqno   = send_seqno[vc->lpid]++;
#ifdef ENABLED_CHECKPOINTING
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif
    MPID_NEM_MEMCPY (el->pkt.mpich2.payload, buf, size);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if(MPID_NEM_IS_LOCAL (vc->lpid))
    {
	MPID_nem_queue_enqueue( MPID_nem_mem_region.RecvQ[vc->lpid], el);
	/*MPID_nem_rel_dump_queue( MPID_nem_mem_region.RecvQ[vc->lpid] ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, size);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues12));
    DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /*PREFETCH_CELL    */


    /*DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14)); */
    return MPID_NEM_MPICH2_SUCCESS;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS
/* same as above, but sends MPICH2 32 byte header */
int
MPID_nem_mpich2_send_header (void* buf, int size, MPIDI_VC_t *vc)
{
    MPID_nem_cell_ptr_t el;
    int my_rank;

#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    
    /*DO_PAPI (PAPI_reset (PAPI_EventSet)); */

    assert (size == MPID_NEM__MPICH2_HEADER_LEN);

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (MPID_NEM_IS_LOCAL (vc->lpid))
    {
	MPID_nem_fbox_mpich2_t *pbox = &MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[vc->lpid]]->mpich2;
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
	    pbox->cell.pkt.mpich2.seqno   = send_seqno[dest]++;
#if MPID_NEM__MPICH2_HEADER_LEN < 32
#error Cant handle case for MPICH2_HEADER_LEN < 32
#endif
#ifdef ENABLED_CHECKPOINTING
	    pbox->cell.pkt.mpich2.datalen = size;
	    pbox->cell.pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif /* ENABLED_CHECKPOINTING */
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
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	    return MPID_NEM_MPICH2_AGAIN;
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));
#else /* PREFETCH_CELL */
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
    {
	return MPID_NEM_MPICH2_AGAIN;
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank] , &el);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /* PREFETCH_CELL */

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    el->pkt.mpich2.source  = my_rank;
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = size;
    el->pkt.mpich2.seqno   = send_seqno[vc->lpid]++;
#ifdef ENABLED_CHECKPOINTING
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
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11));

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (MPID_NEM_IS_LOCAL (vc->lpid))
    {
	MPID_nem_queue_enqueue( MPID_nem_mem_region.RecvQ[vc->lpid], el);
	/*MPID_nem_rel_dump_queue( MPID_nem_mem_region.RecvQ[vc->lpid] ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, size);
    }
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues12));
    DO_PAPI (PAPI_reset (PAPI_EventSet));    

#ifdef PREFETCH_CELL
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues10));
#endif /*PREFETCH_CELL */

    /*DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues14)); */
    return MPID_NEM_MPICH2_SUCCESS;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS
/*
  int MPID_nem_mpich2_sendv (struct iovec **iov, int *n_iov, int vc);

  sends iov to vc
  Non-blocking
  if iov specifies more than MPID_NEM_MPICH2_DATA_LEN of data, the iov will be truncated, so that after MPID_nem_mpich2_sendv returns,
  iov will describe unsent data
  returns MPID_NEM_MPICH2_AGAIN if it can't get a free cell
*/
int
MPID_nem_mpich2_sendv (struct iovec **iov, int *n_iov, int vc)
{
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    int payload_len;    
    int my_rank;

#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));

    my_rank = MPID_nem_mem_region.rank;
	
#ifdef PREFETCH_CELL
    el = prefetched_cell;
    
    if (!el)
    {
	if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	{
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
	    return MPID_NEM_MPICH2_AGAIN;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
#else /*PREFETCH_CELL     */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
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
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = MPID_NEM_MPICH2_DATA_LEN - payload_len;
    el->pkt.mpich2.seqno   = send_seqno[vc->lpid]++;
#ifdef ENABLED_CHECKPOINTING
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif

    if(MPID_NEM_IS_LOCAL (vc->lpid))
    {
	MPID_nem_queue_enqueue (MPID_nem_mem_region.RecvQ[vc->lpid], el);
	/*MPID_nem_rel_dump_queue( MPID_nem_mem_region.RecvQ[vc->lpid] ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, MPID_NEM_MPICH2_DATA_LEN - payload_len);
    }

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
    return MPID_NEM_MPICH2_SUCCESS;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS
/* same as above but first iov element is an MPICH2 32 byte header */
int
MPID_nem_mpich2_sendv_header (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc)
{
    MPID_nem_cell_ptr_t el;
    char *cell_buf;
    int payload_len;    
    int my_rank;

#ifdef ENABLED_CHECKPOINTING
    if (MPID_nem_ckpt_sending_markers)
    {
	MPID_nem_ckpt_send_markers();
	return MPID_NEM_MPICH2_AGAIN;
    }
#endif
    
    DO_PAPI (PAPI_reset (PAPI_EventSet));
    assert (*n_iov >= 1 && (*iov)->iov_len == MPID_NEM__MPICH2_HEADER_LEN);

    my_rank = MPID_nem_mem_region.rank;

#ifdef USE_FASTBOX
    if (MPID_NEM_IS_LOCAL (vc->lpid) && (*n_iov == 2 && (*iov)[1].iov_len + MPID_NEM__MPICH2_HEADER_LEN <= MPID_NEM_FBOX_DATALEN))
    {
	MPID_nem_fbox_mpich2_t *pbox = &MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[vc->lpid]]->mpich2;
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
	    pbox->cell.pkt.mpich2.datalen = (*iov)[1].iov_len + MPID_NEM__MPICH2_HEADER_LEN;
	    pbox->cell.pkt.mpich2.seqno   = send_seqno[vc->lpid]++;
#if MPID_NEM__MPICH2_HEADER_LEN < 32
#error Cant handle case for MPICH2_HEADER_LEN < 32
#endif
#ifdef ENABLED_CHECKPOINTING
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
	    DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues9));

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
	    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
	    return MPID_NEM_MPICH2_AGAIN;
	}
	
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
    {
	DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
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
    el->pkt.mpich2.dest    = vc->lpid;
    el->pkt.mpich2.datalen = MPID_NEM_MPICH2_DATA_LEN - payload_len;
    el->pkt.mpich2.seqno   = send_seqno[vc->lpid]++;
#ifdef ENABLED_CHECKPOINTING
    el->pkt.mpich2.type = MPID_NEM_PKT_MPICH2;
#endif

    if (MPID_NEM_IS_LOCAL (vc->lpid))
    {    
	MPID_nem_queue_enqueue (MPID_nem_mem_region.RecvQ[vc->lpid], el);	
	/*MPID_nem_rel_dump_queue( MPID_nem_mem_region.RecvQ[vc->lpid] ); */
    }
    else
    {
        MPID_nem_net_module_send (vc, el, MPID_NEM_MPICH2_DATA_LEN - payload_len);
    }

#ifdef PREFETCH_CELL
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.FreeQ[my_rank]))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.FreeQ[my_rank], &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues5));
    return MPID_NEM_MPICH2_SUCCESS;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS
/*
  MPID_nem_mpich2_dequeue_fastbox (int local_rank)
  decrements usage count on fastbox for process with local rank local_rank and
  dequeues it from fbox queue if usage is 0.
  This function is called whenever a receive for a process on this node is matched.
  Fastboxes on fbox queue are polled regularly for incoming messages.
*/
int
MPID_nem_mpich2_dequeue_fastbox (int local_rank)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
    MPID_nem_fboxq_elem_t *el;

    el = &fboxq_elem_list[local_rank];

    if (!el->usage)
    {
	ret = MPID_NEM_MPICH2_FAILURE;
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
    return ret;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS

/*
  MPID_nem_mpich2_enqueue_fastbox (int local_rank)
  enqueues fastbox for process with local rank local_rank on fbox queue
  This function is called whenever a receive is posted for a process on this node.
  Fastboxes on fbox queue are polled regularly for incoming messages.
*/
int
MPID_nem_mpich2_enqueue_fastbox (int local_rank)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
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
    
    return ret;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS

/*
  recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
  check whether the sequence number for the cell at the head of qhead is the one
  expected from the sender of that cell
*/
int
recv_seqno_matches (MPID_nem_queue_ptr_t qhead)
{
    MPID_nem_cell_ptr_t cell = MPID_NEM_REL_TO_ABS(qhead->my_head);
    int source = cell->pkt.mpich2.source;
    
    return (cell->pkt.mpich2.seqno == recv_seqno[source]);
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS


/*
  int MPID_nem_mpich2_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);

  non-blocking receive
  sets cell to the received cell, or NULL if there is nothing to receive. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich2_release_cell() once the packet has been copied out
*/
int
MPID_nem_mpich2_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int my_rank = MPID_nem_mem_region.rank;

    DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef ENABLED_CHECKPOINTING
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
#ifdef ENABLED_CHECKPOINTING
    if ((*cell)->pkt.header.type == MPID_NEM_PKT_CKPT)
	MPID_nem_ckpt_got_marker (cell, in_fbox);
    else if (MPID_nem_ckpt_logging_messages)
	MPID_nem_ckpt_log_message (*cell);
#endif
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues6));
    return MPID_NEM_MPICH2_SUCCESS;
}
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS

/*
  int MPID_nem_mpich2_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);

  blocking receive with timeout
  waits up to timeout iterations to receive a cell
  sets cell to the received cell, or NULL if there is nothing to receive. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich2_release_cell() once the packet has been copied out
*/
int
MPID_nem_mpich2_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout)
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
#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS


/*
  int MPID_nem_mpich2_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);

  blocking receive
  waits until there is something to receive, then sets cell to the received cell. in_fbox is true iff the cell was found in a fbox
  the cell must be released back to the subsystem with MPID_nem_mpich2_release_cell() once the packet has been copied out
*/
int
MPID_nem_mpich2_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox)
{
    int my_rank = MPID_nem_mem_region.rank;
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
	DO_PAPI (PAPI_reset (PAPI_EventSet));

#ifdef USE_FASTBOX	
	poll_all_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
	poll_fboxes (cell, {*in_fbox = 1; goto exit_l;} );
#endif /*USE_FASTBOX */

	if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
	{
	    MPID_nem_network_poll (MPID_NEM_POLL_IN);
	}
#ifndef ENABLE_NO_SCHED_YIELD
	if (pollcount >= POLLS_BEFORE_YIELD)
	{
	    pollcount = 0;
	    sched_yield();
	}
	++pollcount;
#endif
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.RecvQ[my_rank], cell);

    ++recv_seqno[(*cell)->pkt.mpich2.source];
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
    return MPID_NEM_MPICH2_SUCCESS;
}


#endif
#ifndef MPID_NEM_DONT_INLINE_FUNCTIONS
/*
  int MPID_nem_mpich2_release_cell (MPID_nem_cell_ptr_t cell, int from);

  releases the cell back to the subsystem to be used for subsequent receives
*/
int
MPID_nem_mpich2_release_cell (MPID_nem_cell_ptr_t cell)
{
    int source = cell->pkt.mpich2.source;
    DO_PAPI (PAPI_reset (PAPI_EventSet));
#ifdef ENABLED_CHECKPOINTING
    if (cell->pkt.header.type == MPID_NEM_PKT_CKPT_REPLAY)
    {
	if (!MPID_nem_ckpt_message_log)
	    /* this is the last replayed message */
	    MPID_nem_ckpt_free_msg_log();
	return MPID_NEM_MPICH2_SUCCESS;
    }
#endif
    MPID_nem_queue_enqueue (MPID_nem_mem_region.FreeQ[source], cell);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet,PAPI_vvalues9));
    return MPID_NEM_MPICH2_SUCCESS;
}
#endif

/*
  int MPID_nem_mpich2_release_fbox (MPID_nem_cell_ptr_t cell);
  releases the fbox back to the subsystem to be used for subsequent receives
*/
/*
  int 
  MPID_nem_mpich2_release_fbox (MPID_nem_cell_ptr_t cell) 
  { 
  DO_PAPI (PAPI_reset (PAPI_EventSet)); 
  MPID_nem_mem_region.mailboxes.in[cell->pkt.mpich2.source]->mpich2.flag.value = 0; 
  DO_PAPI (PAPI_accum_var (PAPI_EventSet,PAPI_vvalues9)); 
  return MPID_NEM_MPICH2_SUCCESS; 
  }
*/


#if 0
int
MPID_nem_mpich2_lmt_send_pre (struct iovec *iov, int n_iov, int dest, struct iovec *cookie)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
    
    if (!MPID_NEM_IS_LOCAL (dest))
    {
	if (gm_module_lmt_send_pre (iov, n_iov, dest, cookie) != 0)
	{
	    ret = MPID_NEM_MPICH2_FAILURE;
	}
    }	    
    return ret;
}

int
MPID_nem_mpich2_lmt_recv_pre (struct iovec *iov, int n_iov, int src, struct iovec *cookie)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
    
    if (!MPID_NEM_IS_LOCAL (src))
    {
	if (gm_module_lmt_recv_pre (iov, n_iov, src, cookie) != 0)
	{
	    ret = MPID_NEM_MPICH2_FAILURE;
	}
    }
	    
    return ret;
}

int
MPID_nem_mpich2_lmt_start_send (int dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
    
    if (!MPID_NEM_IS_LOCAL (dest))
    {
	gm_module_lmt_start_send (dest, s_cookie, r_cookie, completion_ctr);
    }
    
    return ret;
}

int
MPID_nem_mpich2_lmt_start_recv (int src, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
    
    if (!MPID_NEM_IS_LOCAL (src))
    {
	gm_module_lmt_start_recv (src, s_cookie, r_cookie, completion_ctr);
    }
    
    return ret;
}

int
MPID_nem_mpich2_lmt_send_post (int dest, struct iovec cookie)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;
    
    if (!MPID_NEM_IS_LOCAL (dest))
    {
	if (gm_module_lmt_send_post (cookie) != 0)
	{
	    ret = MPID_NEM_MPICH2_FAILURE;
	}
    }
    
    return ret;
}

int
MPID_nem_mpich2_lmt_recv_post (int src, struct iovec cookie)
{
    int ret = MPID_NEM_MPICH2_SUCCESS;

    if (!MPID_NEM_IS_LOCAL (src))
    {
	if (gm_module_lmt_send_post (cookie) != 0)
	{
	    ret = MPID_NEM_MPICH2_FAILURE;
	}
    }
    
    return ret;
}
#endif

