/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <errno.h>
#include <sys/uio.h>

#define MAX_SEND_IOV 10

struct {MPIDI_VC_t *head;} MPID_nem_tcp_module_send_list = {0};
struct {MPIDI_VC_t *head, *tail;} MPID_nem_newtcp_module_free_buffers = {0};

#define ALLOC_Q_ELEMENT(e) do {                                                                                                 \
        if (Q_EMPTY (MPID_nem_newtcp_module_free_buffers))                                                                      \
        {                                                                                                                       \
            MPIU_CHKPMEM_MALLOC (*(e), MPID_nem_newtcp_module_send_q_element_t, sizeof(send_queue_element_t), mpi_errno,        \
                                 "send queue element");                                                                         \
        }                                                                                                                       \
        else                                                                                                                    \
        {                                                                                                                       \
            Q_DEQUEUE (&MPID_nem_newtcp_module_free_buffers, e);                                                                \
        }                                                                                                                       \
    } while (0)

/* FREE_Q_ELEMENTS() frees a list if elements starting at e0 through e1 */
#define FREE_Q_ELEMENTS(e0, e1) Q_ENQUEUE_MULTIPLE (&MPID_nem_newtcp_module_free_buffers, e0, e1)
#define FREE_Q_ELEMENT(e) Q_ENQUEUE (&MPID_nem_newtcp_module_free_buffers, e) 


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    size_t offset;
    MPID_nem_pkt_t *pkt;
    MPID_nem_newtcp_module_send_q_element_t *e;
    MPIU_CHKPMEM_DECL(2);

    pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */

    pkt->mpich2.datalen = datalen;
    pkt->mpich2.source  = MPID_nem_mem_region.rank;    

    if (!vc_is_connected (vc))
    {
        MPID_nem_newtcp_module_connection_progress (vc); /* try to get connected */
        if (!vc_is_connected (vc))
        {
            goto enqueue_cell_and_exit;
        }
    }
    
     if (!Q_EMPTY (vc_ch->send_queue))
    {
        MPID_nem_newtcp_module_send_queue (vc); /* try to empty the queue */
        if (!Q_EMPTY (vc_ch->send_queue))
        {
            goto enqueue_cell_and_exit;
        }
    }

    /* start sending the cell */

    CHECK_EINTR (offset, write (vc_ch->fd, pkt, MPID_NEM_PACKET_LEN (pkt)));
    MPIU_ERR_CHKANDJUMP1 (offset == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**write", "**write %s", strerror (errno));

    if (offset == MPID_NEM_PACKET_LEN (pkt))
        goto fn_exit; /* whole pkt has been sent, we're done */

    /* part of the pkt wasn't sent, enqueue it */
    ALLOC_Q_ELEMENT (&e);
    MPID_NEM_MEMCPY (&e->buf, (char *)pkt + offset, MPID_NEM_PACKET_LEN (pkt));
    e->len = MPID_NEM_PACKET_LEN (pkt) - offset;
    e->start = e->buf;
    
    Q_ENQUEUE_EMPTY (&vc_ch->send_queue, e);
    VC_L_ADD (&MPID_nem_tcp_module_send_list, vc);
    
 fn_exit:
    MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);
    MPIU_CHKPMEM_COMMIT();    
    return mpi_errno;
 fn_enqueue_and_exit:
    /* enqueue cell on send queue and exit */
    ALLOC_Q_ELEMENT (&e);
    MPID_NEM_MEMCPY (&e->buf, pkt, MPID_NEM_PACKET_LEN (pkt));
    e->len = MPID_NEM_PACKET_LEN (pkt);
    e->start = e->buf;
    Q_ENQUEUE (&vc_ch->send_queue, e);
    goto fn_exit;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_send_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_send_queue (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_IOV iov[MAX_SEND_IOV];
    int count;
    MPID_nem_newtcp_module_send_q_element_t *e, *e_first, *e_last;
    ssize_t bytes_sent;
    ssize_t bytes_queued;    

    if (Q_EMPTY (vc_ch->send_queue))
        goto fn_exit;

    /* construct iov of pending sends */
    bytes_queued = 0;
    count = 0;
    e_last = NULL;
    e = Q_HEAD (vc_ch->send_queue);
    do
    {
        iov[count].MPID_IOV_BUF = e->start;
        iov[count].MPID_IOV_LEN = e->len;

        bytes_queued += e->len;
        ++count;
        e_last = e;
        e = e->next;
    }
    while (count < MAX_SEND_IOV && e->next);

    /* write iov */
    CHECK_EINTR (bytes_sent, writev (vc_ch->fd, iov, count));
    MPIU_ERR_CHKANDJUMP1 (bytes_sent == -1 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**writev", "**writev %s", strerror (errno));

    /* remove pending sends that were sent */

    if (bytes_sent == 0) /* nothing was sent */
        goto fn_exit;

    e_first = Q_HEAD (vc_ch->send_queue);
    if (bytes_sent == bytes_queued) /* everything was sent */
    {
        Q_REMOVE_ELEMENTS (&vc_ch->send_queue, e_first, e_last);
        FREE_Q_ELEMENTS (e_first, e_last);

        if (Q_EMPTY (vc_ch->send_queue))
        {
            VC_L_REMOVE (&MPID_nem_tcp_module_send_list, vc);
        }

        goto fn_exit;
    }
    
    e_last = NULL;
    e = Q_HEAD (vc_ch->send_queue);
    do
    {
        if (e->len < bytes_sent)
        {
            bytes_sent -= e->len;
            e_last = e;
            e = e->next;
        }
        else if (e->len > bytes_sent)
        {
            e->len -= bytes_sent;
            e->start += bytes_sent;
            break;
        }
        else /* (e->len == bytes_sent) */
        {
            e_last = e;
            break;
        }
    }
    while (count < MAX_SEND_IOV && e->next);

    if (e_last != NULL) /* did we send at least one queued send? */
    {
        Q_REMOVE_ELEMENTS (vc_ch->send_queue, e_first, e_last);
        FREE_Q_ELEMENTS (e_first, e_last);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    return mpi_errno;
}

