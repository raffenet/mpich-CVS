/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

extern void *MPIDI_CH3_packet_buffer;
extern int MPIDI_CH3I_my_rank;

#define CH3_NORMAL_QUEUE 0
#define CH3_RNDV_QUEUE   1
#define CH3_NUM_QUEUES   2

extern struct MPID_Request *MPIDI_CH3I_sendq_head[CH3_NUM_QUEUES];
extern struct MPID_Request *MPIDI_CH3I_sendq_tail[CH3_NUM_QUEUES];
extern struct MPID_Request *MPIDI_CH3I_active_send[CH3_NUM_QUEUES];

#define MPIDI_CH3I_SendQ_enqueue(req, queue)					\
{										\
    /* MT - not thread safe! */							\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue req=0x%08x", req->handle));	\
    req->dev.next = NULL;							\
    if (MPIDI_CH3I_sendq_tail[queue] != NULL)					\
    {										\
	MPIDI_CH3I_sendq_tail[queue]->dev.next = req;				\
    }										\
    else									\
    {										\
	MPIDI_CH3I_sendq_head[queue] = req;					\
    }										\
    MPIDI_CH3I_sendq_tail[queue] = req;						\
}

#define MPIDI_CH3I_SendQ_dequeue(queue)						\
{										\
    /* MT - not thread safe! */							\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue req=0x%08x",			\
                      MPIDI_CH3I_sendq_head[queue]->handle));			\
    MPIDI_CH3I_sendq_head[queue] = MPIDI_CH3I_sendq_head[queue]->dev.next;	\
    if (MPIDI_CH3I_sendq_head[queue] == NULL)					\
    {										\
	MPIDI_CH3I_sendq_tail[queue] = NULL;					\
    }										\
}

#define MPIDI_CH3I_SendQ_head(queue) (MPIDI_CH3I_sendq_head[queue])

#define MPIDI_CH3I_SendQ_empty(queue) (MPIDI_CH3I_sendq_head[queue] == NULL)


int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);

int MPIDI_CH3I_SendEagerNoncontig( MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz );

int MPID_nem_lmt_shm_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPID_Request *req);
int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie);
int MPID_nem_lmt_shm_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie);
int MPID_nem_lmt_shm_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie);
int MPID_nem_lmt_shm_done_send(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_shm_done_recv(MPIDI_VC_t *vc, MPID_Request *req);

/* #define BYPASS_PROGRESS */

/* short MPIDI_CH3I_Listener_get_port(void); */
/* int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t *); */
/* int MPIDI_CH3I_VC_post_read(MPIDI_VC_t *, MPID_Request *); */
/* int MPIDI_CH3I_VC_post_write(MPIDI_VC_t *, MPID_Request *); */
/* int MPIDI_CH3I_sock_errno_to_mpi_errno(char * fcname, int sock_errno); */
/* int MPIDI_CH3I_Get_business_card(char *value, int length); */



#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
