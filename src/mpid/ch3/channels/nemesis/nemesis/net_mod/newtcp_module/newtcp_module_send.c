/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"

#define MAX_SEND_IOV 10


static int set_plfd_and_poke(MPIDI_VC_t *vc);

int MPID_nem_newtcp_module_send_queued(MPIDI_VC_t *vc);


static inline void sendq_enqueue(MPID_nem_newtcp_send_queue_t *qhead, MPID_Request *element)
{
    volatile MPID_Request *prev;

    prev = MPID_NEM_SWAP(&qhead->tail, element);

    if (prev == NULL)
	qhead->head = element;
    else
	prev->ch.next = element;
}

static inline MPID_Request *sendq_head(MPID_nem_newtcp_send_queue_t *qhead)
{
    if (qhead->my_head == NULL)
    {
	if (qhead->head == NULL)
	    return NULL;
	else
	{
	    qhead->my_head = qhead->head;
	    qhead->head = NULL; /* reset it for next time */
	}
    }
    return qhead->my_head;
}

static inline int sendq_empty(MPID_nem_newtcp_send_queue_t *qhead)
{
    if (qhead->my_head == NULL)
    {
	if (qhead->head == NULL)
	    return 1;
	else
	{
	    qhead->my_head = qhead->head;
	    qhead->head = NULL; /* reset it for next time */
	}
    }
    return 0;
}

static inline void sendq_dequeue(MPID_nem_newtcp_send_queue_t *qhead, MPID_Request **element)
{
    volatile MPID_Request *e;
    e = qhead->my_head;

    if (e->ch.next)
	qhead->my_head = e->ch.next;
    else
    {
	MPID_Request *old_tail;
      
	qhead->my_head = NULL;	

	old_tail = MPID_NEM_CAS(&qhead->tail, e, NULL);

	if (old_tail != e)
	{
	    while (e->ch.next = NULL)
		SKIP;
	    qhead->my_head = e->ch.next;
	}
    }
    e->ch.next = NULL;
    *element = e;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_INIT);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_Assert(0);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_queued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_queued(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *sreq;
    MPIDI_msg_sz_t offset;
    MPID_IOV *iov;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_QUEUED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_QUEUED);

    while (!sendq_empty(VC_FIELD(vc, send_queue)))
    {
        sreq = sendq_head(VC_FIELD(vc, send_queue));
        
        iov = &sreq->dev.iov[sreq->ch.iov_offset];

/*         printf("sreq = %p sreq->dev.iov = %p iov = %p\n", sreq, sreq->dev.iov, iov); */
/*         printf("iov[0].MPID_IOV_BUF = %p iov[0].MPID_IOV_LEN = %d iov_count = %d\n", iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN, sreq->dev.iov_count);//DARIUS */
/*         printf("&iov[0].MPID_IOV_LEN = %p sreq->ch.iov_offset = %d\n", &iov[0].MPID_IOV_LEN, sreq->ch.iov_offset);//DARIUS */
        CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, sreq->dev.iov_count));
        MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        if (offset == -1)
        {
            if (errno == EAGAIN)
                offset = 0;
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

        complete = 1;
        for (iov = &sreq->dev.iov[sreq->ch.iov_offset]; iov < &sreq->dev.iov[sreq->ch.iov_offset + sreq->dev.iov_count]; ++iov)
        {
            if (offset < iov->MPID_IOV_LEN)
            {
                iov->MPID_IOV_BUF = (char *)iov->MPID_IOV_BUF + offset;
                iov->MPID_IOV_LEN -= offset;
                sreq->ch.iov_offset = iov - sreq->dev.iov;
                complete = 0;
                break;
            }
            offset -= iov->MPID_IOV_LEN;
        }
        if (complete)
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                sendq_dequeue(VC_FIELD(vc, send_queue), &sreq);
                continue;
            }

            complete = 0;
            mpi_errno = reqFn(vc, sreq, &complete);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            if (complete)
            {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                sendq_dequeue(VC_FIELD(vc, send_queue), &sreq);
                continue;
            }
            sreq->ch.iov_offset = 0;
        }
    }

    if (sendq_empty(VC_FIELD(vc, send_queue)))
        UNSET_PLFD(vc);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_QUEUED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_FINALIZE);


    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_SEND_FINALIZE);
    return mpi_errno;
}

/* MPID_nem_newtcp_module_conn_est -- this function is called when the
   connection is finally extablished to send any pending sends */
#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_conn_est
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_conn_est (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONN_EST);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONN_EST);

/*     printf ("*** connected *** %d\n", VC_FIELD(vc, sc)->fd); //DARIUS */

    if (!sendq_empty(VC_FIELD(vc, send_queue)))
    {
        SET_PLFD(vc);
        mpi_errno = MPID_nem_newtcp_module_send_queued(vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_MODULE_CONN_EST);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_iStartContigMsg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    MPIDI_msg_sz_t offset = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iStartContigMsg");
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (!MPID_nem_newtcp_module_vc_is_disconnected(vc) && sendq_empty(VC_FIELD(vc, send_queue)))
    {
        MPID_IOV iov[2];

        iov[0].MPID_IOV_BUF = hdr;
        iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
        iov[1].MPID_IOV_BUF = data;
        iov[1].MPID_IOV_LEN = data_sz;
        
        CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, 2));
        MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        if (offset == -1)
        {
            if (errno == EAGAIN)
                offset = 0;
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

        if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
        {
            /* sent whole message */
            *sreq_ptr = NULL;
            goto fn_exit;
        }
    }
    

    /* create and enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    /* create a request */
    sreq = MPID_Request_create();
    MPIU_Assert (sreq != NULL);
    MPIU_Object_set_ref (sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = 0;
    sreq->ch.vc = vc;
    sreq->ch.iov_offset = 0;

    /*     printf("&sreq->dev.pending_pkt = %p sizeof(MPIDI_CH3_PktGeneric_t) = %d\n", &sreq->dev.pending_pkt, sizeof(MPIDI_CH3_PktGeneric_t));//DARIUS */
    /*     printf("offset = %d\n", offset);//DARIUS */

    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset ;
        if (data_sz)
        {
            sreq->dev.iov[1].MPID_IOV_BUF = data;
            sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }
    
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);

    /*     printf("sreq = %p sreq->dev.iov = %p\n", sreq, sreq->dev.iov); */
    /*     printf("sreq->dev.iov[0].MPID_IOV_BUF = %p\n", sreq->dev.iov[0].MPID_IOV_BUF);//DARIUS */
    /*     printf("sreq->dev.iov[0].MPID_IOV_LEN = %d\n", sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */
    /*     printf("&sreq->dev.iov[0].MPID_IOV_LEN = %p\n", &sreq->dev.iov[0].MPID_IOV_LEN);//DARIUS */

    if (sendq_empty(VC_FIELD(vc, send_queue)))
    {
        sendq_enqueue(VC_FIELD(vc, send_queue), sreq);

        if (MPID_nem_newtcp_module_vc_is_disconnected(vc))
        {
            mpi_errno = signal_comm_thread_to_connect(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else
            set_plfd_and_poke(vc);
    }
    else
        sendq_enqueue(VC_FIELD(vc, send_queue), sreq);

    *sreq_ptr = sreq;
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISTARTCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_iSendContig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t offset = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iSendContig");

    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (!MPID_nem_newtcp_module_vc_is_disconnected(vc) && sendq_empty(VC_FIELD(vc, send_queue)))
    {
        MPID_IOV iov[2];

        iov[0].MPID_IOV_BUF = hdr;
        iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
        iov[1].MPID_IOV_BUF = data;
        iov[1].MPID_IOV_LEN = data_sz;
        
        CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, 2));
        MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        if (offset == -1)
        {
            if (errno == EAGAIN)
                offset = 0;
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write %d", offset);

        if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
        {
            /* sent whole message */
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

            reqFn = sreq->dev.OnDataAvail;
            if (!reqFn)
            {
                MPIU_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                MPIDI_CH3U_Request_complete(sreq);
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                goto fn_exit;
            }
            else
            {
                int complete = 0;
                
                mpi_errno = reqFn(vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (complete)
                {
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    goto fn_exit;
                }

                /* not completed: more to send */
                goto enqueue_request;
            }
        }
    }

    /* save iov */
    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset;
        if (data_sz)
        {
            sreq->dev.iov[1].MPID_IOV_BUF = data;
            sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
            sreq->dev.iov_count = 2;
        }
        else
            sreq->dev.iov_count = 1;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }

 enqueue_request:
    /* enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);

    sreq->ch.vc = vc;
    sreq->ch.iov_offset = 0;

    if (sendq_empty(VC_FIELD(vc, send_queue)))
    {
        sendq_enqueue(VC_FIELD(vc, send_queue), sreq);

        if (MPID_nem_newtcp_module_vc_is_disconnected(vc))
        {
            mpi_errno = signal_comm_thread_to_connect(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else
            set_plfd_and_poke(vc);
    }
    else
        sendq_enqueue(VC_FIELD(vc, send_queue), sreq);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_SendEagerNoncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_SendEagerNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int iov_n;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPID_IOV *iov_p;
    MPIDI_msg_sz_t offset;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_SENDEAGERNONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_SENDEAGERNONCONTIG);

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_SendEagerNoncontig");
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t));
    
    iov[0].MPID_IOV_BUF = header;
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    iov_n = MPID_IOV_LIMIT - 1;
    /* On the initial load of a send iov req, set the OnFinal action (null
       for point-to-point) */
    sreq->dev.OnFinal = 0;
    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");

    iov_n += 1;
    offset = 0;

    if (!MPID_nem_newtcp_module_vc_is_disconnected(vc) && sendq_empty(VC_FIELD(vc, send_queue)))
    {
        CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, iov_n));
        MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        if (offset == -1)
        {
            if (errno == EAGAIN)
                offset = 0;
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
        }
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "write noncontig %d", offset);
    }

    if (offset < iov[0].MPID_IOV_LEN)
    {
        /* header was not yet sent, save it in req */
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
        iov[0].MPID_IOV_BUF = &sreq->dev.pending_pkt;
        iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
    }

    /* check if whole iov was sent, and save any unsent portion of iov */
    sreq->dev.iov_count = 0;
    complete = 1;
    for (iov_p = &iov[0]; iov_p < &iov[iov_n]; ++iov_p)
    {
        if (offset < iov_p->MPID_IOV_LEN)
        {
            sreq->dev.iov[sreq->dev.iov_count].MPID_IOV_BUF = (char *)iov_p->MPID_IOV_BUF + offset;
            sreq->dev.iov[sreq->dev.iov_count].MPID_IOV_LEN = iov_p->MPID_IOV_LEN - offset;
            offset = 0;
            ++sreq->dev.iov_count;
            complete = 0;
        }
        else
            offset -= iov_p->MPID_IOV_LEN;
    }
        
    if (complete)
    {
        /* sent whole iov */
        int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);

        reqFn = sreq->dev.OnDataAvail;
        if (!reqFn)
        {
            MPIDI_CH3U_Request_complete(sreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        }

        complete = 0;
        mpi_errno = reqFn(vc, sreq, &complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
        if (complete)
        {
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            goto fn_exit;
        }
    }
        
    /* enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");
    MPIU_Assert(sreq->dev.iov_count >= 1 && sreq->dev.iov[0].MPID_IOV_LEN > 0);
        
    sreq->ch.vc = vc;
    sreq->ch.iov_offset = 0;

    if (sendq_empty(VC_FIELD(vc, send_queue)))
    {
        sendq_enqueue(VC_FIELD(vc, send_queue), sreq);

        if (MPID_nem_newtcp_module_vc_is_disconnected(vc))
        {
            mpi_errno = signal_comm_thread_to_connect(vc);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else
            set_plfd_and_poke(vc);
    }
    else
        sendq_enqueue(VC_FIELD(vc, send_queue), sreq);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_SENDEAGERNONCONTIG);
    return mpi_errno;
 fn_fail:
    MPIU_Object_set_ref(sreq, 0);
    MPIDI_CH3_Request_destroy(sreq);
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME signal_comm_thread_to_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int signal_comm_thread_to_connect(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_newtcp_module_poke_msg_t msg;
    MPIDI_msg_sz_t count;
    MPIDI_CH3I_VC * const vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_SIGNAL_COMM_THREAD_TO_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SIGNAL_COMM_THREAD_TO_CONNECT);

    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "connecting to %d", vc->pg_rank);

/*     if (VC_FIELD(vc, sc) != NULL) //DARIUS */
/*         MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "*****************STUCK***********"); //DARIUS */
/*     while (VC_FIELD(vc, sc) != NULL) ;//DARIUS */
    
/*     MPIU_Assert(VC_FIELD(vc, sc) == NULL); */
    MPIU_Assert(vc_ch->state == MPID_NEM_VC_STATE_DISCONNECTED);

    vc_ch->state = MPID_NEM_VC_STATE_CONNECTING;

    msg.type = MPID_NEM_NEWTCP_MODULE_CONNECT;
    msg.vc = vc;

    CHECK_EINTR(count, write(MPID_nem_newtcp_module_main_to_comm_fd, &msg, sizeof(msg)));
    MPIU_ERR_CHKANDJUMP1(count == -1, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror(errno));
    MPIU_ERR_CHKANDJUMP1(count != sizeof(msg), mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "couldn't send whole message");

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SIGNAL_COMM_THREAD_TO_CONNECT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME set_plfd_and_poke
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int set_plfd_and_poke(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_newtcp_module_poke_msg_t msg;
    MPIDI_msg_sz_t count;
    MPIDI_STATE_DECL(MPID_STATE_SET_PLFD_AND_POKE);

    MPIDI_FUNC_ENTER(MPID_STATE_SET_PLFD_AND_POKE);

    SET_PLFD(vc);
    
    msg.type = MPID_NEM_NEWTCP_MODULE_REFRESH_STATE;
    msg.vc = vc;

    CHECK_EINTR(count, write(MPID_nem_newtcp_module_main_to_comm_fd, &msg, sizeof(msg)));
    MPIU_ERR_CHKANDJUMP1(count == -1, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror(errno));
    MPIU_ERR_CHKANDJUMP1(count != sizeof(msg), mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "couldn't send whole message");
    
 fn_exit:
     MPIDI_FUNC_EXIT(MPID_STATE_SET_PLFD_AND_POKE);
   return mpi_errno;
 fn_fail:
    goto fn_exit;
}

