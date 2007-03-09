/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"

//#define printf(x...) do {} while(0)

#define NUM_PREALLOC_SENDQ 10
#define MAX_SEND_IOV 10

#define SENDQ_EMPTY(q) GENERIC_Q_EMPTY (q)
#define SENDQ_HEAD(q) GENERIC_Q_HEAD (q)
#define SENDQ_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE (qp, ep, dev.next)
#define SENDQ_DEQUEUE(qp, ep) GENERIC_Q_DEQUEUE (qp, ep, dev.next)


typedef struct MPID_nem_newtcp_module_send_q_element
{
    struct MPID_nem_newtcp_module_send_q_element *next;
    size_t len;                        /* number of bytes left to send */
    char *start;                       /* pointer to next byte to send */
    MPID_nem_cell_ptr_t cell;
    /*     char buf[MPID_NEM_MAX_PACKET_LEN];*/ /* data to be sent */
} MPID_nem_newtcp_module_send_q_element_t;

struct {MPIDI_VC_t *head;} send_list = {0};
struct {MPID_nem_newtcp_module_send_q_element_t *top;} free_buffers = {0};

#define ALLOC_Q_ELEMENT(e) do {                                                                                                         \
        if (S_EMPTY (free_buffers))                                                                                                     \
        {                                                                                                                               \
            MPIU_CHKPMEM_MALLOC (*(e), MPID_nem_newtcp_module_send_q_element_t *, sizeof(MPID_nem_newtcp_module_send_q_element_t),      \
                                 mpi_errno, "send queue element");                                                                      \
        }                                                                                                                               \
        else                                                                                                                            \
        {                                                                                                                               \
            S_POP (&free_buffers, e);                                                                                                   \
        }                                                                                                                               \
    } while (0)

/* FREE_Q_ELEMENTS() frees a list if elements starting at e0 through e1 */
#define FREE_Q_ELEMENTS(e0, e1) S_PUSH_MULTIPLE (&free_buffers, e0, e1)
#define FREE_Q_ELEMENT(e) S_PUSH (&free_buffers, e)

static int send_queued (MPIDI_VC_t *vc);

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_init()
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIU_CHKPMEM_DECL (NUM_PREALLOC_SENDQ);
    
    /* preallocate sendq elements */
    for (i = 0; i < NUM_PREALLOC_SENDQ; ++i)
    {
        MPID_nem_newtcp_module_send_q_element_t *e;
        
        MPIU_CHKPMEM_MALLOC (e, MPID_nem_newtcp_module_send_q_element_t *,
                             sizeof(MPID_nem_newtcp_module_send_q_element_t), mpi_errno, "send queue element");
        S_PUSH (&free_buffers, e);
    }

    MPIU_CHKPMEM_COMMIT();
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
 #if 0
   MPIDI_CH3I_VC *vc_ch = &vc->ch;
    size_t offset;
    MPID_nem_pkt_t *pkt;
    MPID_nem_newtcp_module_send_q_element_t *e;
    MPIU_CHKPMEM_DECL(2);

    pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */

    pkt->mpich2.datalen = datalen;
    pkt->mpich2.source  = MPID_nem_mem_region.rank;    

    if (!MPID_nem_newtcp_module_vc_is_connected (vc))
    {
        /* MPID_nem_newtcp_module_connection_progress (vc);  try to get connected  */
        mpi_errno = MPID_nem_newtcp_module_connect (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        /*  FIXME define the use of this and above commented function */

        if (!MPID_nem_newtcp_module_vc_is_connected (vc))
        {
            /*printf ("  vc not connected, enqueuing %d\n", MPID_NEM_PACKET_LEN (pkt));*/
            goto enqueue_cell_and_exit;
        }
    }

/*     while(!Q_EMPTY (VC_FIELD(vc, send_queue)))//DARIUS */
/*     { */
/*         mpi_errno = send_queued (vc); /\* try to empty the queue *\/ */
/*         if (mpi_errno) MPIU_ERR_POP (mpi_errno); */
/*     } */
    
    if (!Q_EMPTY (VC_FIELD(vc, send_queue)))
    {
        mpi_errno = send_queued (vc); /* try to empty the queue */
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        if (!Q_EMPTY (VC_FIELD(vc, send_queue)))
        {
            /*printf ("  send queue not empty, enqueuing %d\n", MPID_NEM_PACKET_LEN (pkt));*/
            goto enqueue_cell_and_exit;
        }
    }

    /* start sending the cell */

    CHECK_EINTR (offset, write (VC_FIELD(vc, sc)->fd, pkt, MPID_NEM_PACKET_LEN (pkt)));
/*     MPIU_Assert (offset != 0);//DARIUS */
    MPIU_ERR_CHKANDJUMP (offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
    if (offset == -1)
    {
        if (errno == EAGAIN)
            offset = 0;
        else
            MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror (errno));
    }
    
    /*if (offset != -1)
      printf("  write  %d (%d)\n", offset, MPID_NEM_PACKET_LEN (pkt));*/

    if (offset == MPID_NEM_PACKET_LEN (pkt))
    {
/*         printf ("sent %d (%d)\n", MPID_NEM_PACKET_LEN (pkt), pkt->header.seqno);//DARIUS */
        MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);
        goto fn_exit; /* whole pkt has been sent, we're done */
    }
    
/*     printf ("sent %d of %d (%d)\n", offset, MPID_NEM_PACKET_LEN (pkt), pkt->header.seqno);//DARIUS */
    /* part of the pkt wasn't sent, enqueue it */
    ALLOC_Q_ELEMENT (&e); /* may call MPIU_CHKPMEM_MALLOC */
    e->cell = cell;
    e->len = MPID_NEM_PACKET_LEN (pkt) - offset;
    e->start = (char *)pkt + offset;
    /*printf ("  enqueuing %d\n", e->len);*/
    
    Q_ENQUEUE_EMPTY (&VC_FIELD(vc, send_queue), e);
    VC_L_ADD (&send_list, vc);
    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();    
    return mpi_errno;
 enqueue_cell_and_exit:
    /* enqueue cell on send queue and exit */
    ALLOC_Q_ELEMENT (&e); /* may call MPIU_CHKPMEM_MALLOC */
    e->cell = cell;
    e->len = MPID_NEM_PACKET_LEN (pkt);
    e->start = (char *)pkt;
    Q_ENQUEUE (&VC_FIELD(vc, send_queue), e);
    goto fn_exit;
 fn_fail:
    MPIU_CHKPMEM_REAP();
/*     printf ("num_elements = %d\n", num_elements);//DARIUS     */
/*     printf ("num_alloced = %d\n", num_alloced);//DARIUS     */
/*     printf ("num_freed = %d\n", num_freed);//DARIUS     */
#endif
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME send_queued
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_queued (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_Request *sreq;
    MPIDI_msg_sz_t offset;
    MPID_IOV *iov;
    int i;
    int complete;

    while (!SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
    {
        sreq = SENDQ_HEAD(VC_FIELD(vc, send_queue));
        MPIU_Assert(sreq->dev.iov_count <= 2);
        
        iov = &sreq->dev.iov[sreq->ch.iov_offset];
        
        CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, sreq->dev.iov_count));
        MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
        if (offset == -1)
        {
            if (errno == EAGAIN)
                offset = 0;
            else
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
        }

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
                break;
            }
            else
            {
                int complete = 0;
                
                mpi_errno = reqFn(vc, sreq, &complete);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                if (complete)
                {
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
                    break;
                }

                MPIU_Assert(0); /* FIXME:  I don't think we should get here with contig messages */
                
                sreq->ch.vc = vc;
                break;
            }
        }
        SENDQ_DEQUEUE(&VC_FIELD(vc, send_queue), &sreq);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = send_list.head; vc; vc = vc->ch.next)
    {
        mpi_errno = send_queued (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
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

/*     printf ("MPID_nem_newtcp_module_send_finalize\n");//DARIUS */
    while (!VC_L_EMPTY (send_list))
        MPID_nem_newtcp_module_send_progress();

    while (!S_EMPTY (free_buffers))
    {
        MPID_nem_newtcp_module_send_q_element_t *e;
        S_POP (&free_buffers, &e);
        MPIU_Free (e);
    }
/*     printf ("  done\n");//DARIUS */

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

/*     printf ("*** connected *** %d\n", VC_FIELD(vc, sc)->fd); //DARIUS     */

    if (!SENDQ_EMPTY (VC_FIELD(vc, send_queue)))
    {
        VC_L_ADD (&send_list, vc);
        mpi_errno = send_queued (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_fail:    
    return mpi_errno;
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
    
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPID_IOV iov[2];
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iStartContigMsg");

            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            iov[1].MPID_IOV_BUF = data;
            iov[2].MPID_IOV_LEN = data_sz;
        
            CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, 2));
            MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            if (offset == -1)
            {
                if (errno == EAGAIN)
                    offset = 0;
                else
                    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
            }

            if (offset == sizeof(MPIDI_CH3_PktGeneric_t) + data_sz)
            {
                /* sent whole message */
                *sreq_ptr = NULL;
                goto fn_exit;
            }
        }
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
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

    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset ;
        sreq->dev.iov[1].MPID_IOV_BUF = data;
        sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }

    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);
    
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
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);
    
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    
    MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *)hdr);
    if (MPID_nem_newtcp_module_vc_is_connected(vc))
    {
        if (SENDQ_EMPTY(VC_FIELD(vc, send_queue)))
        {
            MPID_IOV iov[2];
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "newtcp_iSendContig");

            iov[0].MPID_IOV_BUF = hdr;
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            iov[1].MPID_IOV_BUF = data;
            iov[2].MPID_IOV_LEN = data_sz;
        
            CHECK_EINTR(offset, writev(VC_FIELD(vc, sc)->fd, iov, 2));
            MPIU_ERR_CHKANDJUMP(offset == 0, mpi_errno, MPI_ERR_OTHER, "**sock_closed");
            if (offset == -1)
            {
                if (errno == EAGAIN)
                    offset = 0;
                else
                    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));
            }

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

                    MPIU_Assert(0); /* FIXME:  I don't think we should get here with contig messages */
                
                    sreq->ch.vc = vc;
                    goto fn_exit;
                }
            }
        }
    }
    else
    {
        mpi_errno = MPID_nem_newtcp_module_connect(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* create and enqueue request */
    MPIU_DBG_MSG (CH3_CHANNEL, VERBOSE, "enqueuing");

    sreq->ch.vc = vc;
    sreq->ch.iov_offset = 0;

    if (offset < sizeof(MPIDI_CH3_PktGeneric_t))
    {
        sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt + offset;
        sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t) - offset ;
        sreq->dev.iov[1].MPID_IOV_BUF = data;
        sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
    }
    else
    {
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)data + (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz - (offset - sizeof(MPIDI_CH3_PktGeneric_t));
        sreq->dev.iov_count = 1;
    }

    SENDQ_ENQUEUE(&VC_FIELD(vc, send_queue), sreq);
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_NEWTCP_ISENDCONTIGMSG);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
