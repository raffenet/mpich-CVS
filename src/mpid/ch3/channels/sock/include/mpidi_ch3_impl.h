/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_sock_conf.h"
#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

typedef struct MPIDI_CH3I_Process_s
{
    MPIDI_CH3I_Process_group_t * pg;
    MPIDI_CH3I_Acceptq_t * acceptq_head;
    MPIDI_CH3I_Acceptq_t * acceptq_tail;
#if !defined(MPICH_SINGLE_THREADED)
    MPID_Thread_lock_t acceptq_mutex;
#endif
}
MPIDI_CH3I_Process_t;

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#define MPIDI_CH3I_SendQ_enqueue(vc, req)									\
{														\
    /* MT - not thread safe! */											\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=%p req=0x%08x", vc, req->handle));       	                \
    req->dev.next = NULL;											\
    if (vc->ch.sendq_tail != NULL)										\
    {														\
	vc->ch.sendq_tail->dev.next = req;									\
    }														\
    else													\
    {														\
	vc->ch.sendq_head = req;										\
    }														\
    vc->ch.sendq_tail = req;											\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vc, req)									\
{														\
    /* MT - not thread safe! */											\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=%p req=0x%08x", vc, req->handle));              	\
    req->dev.next = vc->ch.sendq_tail;										\
    if (vc->ch.sendq_tail == NULL)										\
    {														\
	vc->ch.sendq_tail = req;										\
    }														\
    vc->ch.sendq_head = req;											\
}

#define MPIDI_CH3I_SendQ_dequeue(vc)												\
{																\
    /* MT - not thread safe! */													\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=%p req=0x%08x", vc, vc->ch.sendq_head->handle));	                        \
    vc->ch.sendq_head = vc->ch.sendq_head->dev.next;										\
    if (vc->ch.sendq_head == NULL)												\
    {																\
	vc->ch.sendq_tail = NULL;												\
    }																\
}

#define MPIDI_CH3I_SendQ_head(vc) (vc->ch.sendq_head)

#define MPIDI_CH3I_SendQ_empty(vc) (vc->ch.sendq_head == NULL)

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
short MPIDI_CH3I_Listener_get_port(void);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC *);
int MPIDI_CH3I_VC_post_read(MPIDI_VC *, MPID_Request *);
int MPIDI_CH3I_VC_post_write(MPIDI_VC *, MPID_Request *);
int MPIDI_CH3I_Get_business_card(char *value, int length);
int  MPIDI_CH3I_Connect_to_root(char *port_name, MPIDI_VC **new_vc);

int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC *vc);
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_VC **vc);

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
