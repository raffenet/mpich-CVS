/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#ifdef USE_WINCONF_H
#include "winmpidi_ch3i_sock_conf.h"
#include "winmpidi_ch3_conf.h"
#else
#include "mpidi_ch3i_sock_conf.h"
#include "mpidi_ch3_conf.h"
#endif
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
}
MPIDI_CH3I_Process_t;

extern MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

#define MPIDI_CH3I_SendQ_enqueue(vc, req)									\
{														\
    /* MT - not thread safe! */											\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=%p req=0x%08x", vc, req->handle));       	                \
    req->ch3.next = NULL;											\
    if (vc->sc.sendq_tail != NULL)										\
    {														\
	vc->sc.sendq_tail->ch3.next = req;									\
    }														\
    else													\
    {														\
	vc->sc.sendq_head = req;										\
    }														\
    vc->sc.sendq_tail = req;											\
}

#define MPIDI_CH3I_SendQ_enqueue_head(vc, req)									\
{														\
    /* MT - not thread safe! */											\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=%p req=0x%08x", vc, req->handle));              	\
    req->ch3.next = vc->sc.sendq_tail;										\
    if (vc->sc.sendq_tail == NULL)										\
    {														\
	vc->sc.sendq_tail = req;										\
    }														\
    vc->sc.sendq_head = req;											\
}

#define MPIDI_CH3I_SendQ_dequeue(vc)												\
{																\
    /* MT - not thread safe! */													\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=%p req=0x%08x", vc, vc->sc.sendq_head->handle));	                        \
    vc->sc.sendq_head = vc->sc.sendq_head->ch3.next;										\
    if (vc->sc.sendq_head == NULL)												\
    {																\
	vc->sc.sendq_tail = NULL;												\
    }																\
}

#define MPIDI_CH3I_SendQ_head(vc) (vc->sc.sendq_head)

#define MPIDI_CH3I_SendQ_empty(vc) (vc->sc.sendq_head == NULL)


int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
short MPIDI_CH3I_Listener_get_port(void);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC *);
int MPIDI_CH3I_VC_post_read(MPIDI_VC *, MPID_Request *);
int MPIDI_CH3I_VC_post_write(MPIDI_VC *, MPID_Request *);
int MPIDI_CH3I_sock_errno_to_mpi_errno(char * fcname, int sock_errno);
int MPIDI_CH3I_Get_business_card(char *value, int length);

#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
