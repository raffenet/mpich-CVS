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
	send_seqno = MPIU_Malloc (sizeof(*send_seqno) * MPID_nem_mem_region.num_procs);
	recv_seqno = MPIU_Malloc (sizeof(*recv_seqno) * MPID_nem_mem_region.num_procs);
	if (!send_seqno || !recv_seqno)
	    FATAL_ERROR ("malloc failed");

	for (i = 0; i < MPID_nem_mem_region.num_procs; ++i)
	{
	    send_seqno[i] = 0;
	    recv_seqno[i] = 0;
	}
    
	/* set up fbox queue */
	fboxq_elem_list = MPIU_Malloc (MPID_nem_mem_region.num_local * sizeof(MPID_nem_fboxq_elem_t));
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
	if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	    return MPID_NEM_MPICH2_AGAIN;
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
    }
#else /*PREFETCH_CELL    */
    if (MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ)) 
    { 
	return MPID_NEM_MPICH2_AGAIN;
    }

    MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &el);
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
    if (!MPID_nem_queue_empty (MPID_nem_mem_region.my_freeQ))
	MPID_nem_queue_dequeue (MPID_nem_mem_region.my_freeQ, &prefetched_cell);
    else
	prefetched_cell = 0;
#endif /*PREFETCH_CELL */
#endif /*ENABLED_CHECKPOINTING */
    return MPID_NEM_MPICH2_SUCCESS;
}

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

