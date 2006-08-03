/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <errno.h>

typedef struct recv_overflow_buf
{
    char *start;
    int len;
    MPIDI_VC_t *vc;
    char buf[MPID_NEM_MAX_PACKET_LEN];
} recv_overflow_buf_t;

static recv_overflow_buf_t recv_overflow_buf;

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll_init()
{
        int mpi_errno = MPI_SUCCESS;

        recv_overflow_buf.start = NULL;
        recv_overflow_buf.len = 0;
        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* breakout_pkts -- This is called after receiving data into a cell.
   If there were multiple packets received into this cell, this
   function copies any additional packets into their own cells.  If
   there is only a fraction of a packet left, the cell is not enqueued
   onto the process receive queue, but left as a pending receive in
   the VC structure.  If we run out of free cells before all of the
   packets have been copied out, we copy the extra data into the
   recv_overflow_buf.

   NOTE: For performance purposes, the fast-path case (received
   exactly one packet) should be handled before calling this function.
   For this reason, this function won't handle the exactly-one-packet
   case.
*/
#undef FUNCNAME
#define FUNCNAME breakout_pkts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int breakout_pkts (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t v_cell, int len)
{
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_cell_t *cell = (MPID_nem_cell_t *) v_cell;; /* non-volatile cell */
    struct {MPID_nem_abs_cell_t *head, *tail;} cell_queue;
    MPID_nem_pkt_t *next_pkt;

    if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)))
    {
        vc_ch->pending_recv.cell = cell;
        vc_ch->pending_recv.end = (char *)MPID_NEM_CELL_TO_PACKET (cell) + len;
        vc_ch->pending_recv.len = len;
        goto fn_exit;
    }

    /* there is more than one packet in this cell */

    /* we can't enqueue the cell onto the process recv queue yet
       because we haven't copied all of the additional packets out yet
       (in case we're multithreaded).  So we need to put them in a
       separate queue now, and queue them onto the process recv queue
       once were all done. */
    Q_ENQUEUE_EMPTY (&cell_queue, (MPID_nem_abs_cell_t *)cell);
    len -= MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell));
    next_pkt = (MPID_nem_pkt_t *)((char *)MPID_NEM_CELL_TO_PACKET (cell) + MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)));
    
    MPIU_Assert (len > 0); /* This function is only called when there is less than one packet or more than one packet in the cell */
    
    while (!MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
    {
        MPID_nem_queue_dequeue (MPID_nem_newtcp_module_free_queue, &v_cell);
        cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
        
        if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (cell)))
        {
            MPID_NEM_MEMCPY (MPID_NEM_CELL_TO_PACKET (cell), next_pkt, len);
            vc_ch->pending_recv.cell = cell;
            vc_ch->pending_recv.end = (char *)MPID_NEM_CELL_TO_PACKET (cell) + len;
            vc_ch->pending_recv.len = len;
            goto enqueue_and_exit;
        }
        
        MPID_NEM_MEMCPY (MPID_NEM_CELL_TO_PACKET (cell), next_pkt, MPID_NEM_PACKET_LEN (next_pkt));
        Q_ENQUEUE (&cell_queue, (MPID_nem_abs_cell_t *)cell);
        len -= MPID_NEM_PACKET_LEN (next_pkt);
        next_pkt = (MPID_nem_pkt_t *)((char *)next_pkt + MPID_NEM_PACKET_LEN (next_pkt));

        if (len == 0)
            goto enqueue_and_exit;
    }

    /* we ran out of free cells, copy into overflow buffer */
    MPIU_Assert (recv_overflow_buf.start == NULL);
    MPID_NEM_MEMCPY (recv_overflow_buf.buf, next_pkt, len);
    recv_overflow_buf.start = recv_overflow_buf.buf;
    recv_overflow_buf.len = len;
    recv_overflow_buf.vc = vc;
    
 enqueue_and_exit:
    /* enqueue the received cells onto the process receive queue */
    while (!Q_EMPTY (cell_queue))
    {
        Q_DEQUEUE (&cell_queue, ((MPID_nem_abs_cell_t **)&cell));
        MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);
    }

    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_recv_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_recv_handler (struct pollfd *pfd, sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;
    MPIDI_VC_t *vc = sc->vc;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPID_nem_cell_ptr_t v_cell;
    MPID_nem_cell_t *cell; /* non-volatile cell */

    if (vc_ch->pending_recv.cell)
    {
        MPIU_Assert (0); /* FIXME: if there are no free cells, we need
                            to make sure we don't receive more than
                            one packet into a pending_recv cell.  The
                            problem is that if our overflow buffer is
                            already full, we would have no place to
                            copy the extra packets. --DARIUS*/
        
        /* there is a partially received pkt in tmp_cell, continue receiving into it */
        CHECK_EINTR (bytes_recvd, read (vc->ch.fd, vc_ch->pending_recv.end, MPID_NEM_MAX_PACKET_LEN - vc_ch->pending_recv.len));
        if (bytes_recvd == -1)
        {
            if (errno == EAGAIN)
                goto fn_exit;
            else
                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
        }

        vc_ch->pending_recv.cell = NULL;
        
        /* fast path: single packet case */
        if (vc_ch->pending_recv.len + bytes_recvd >= MPID_NEM_MIN_PACKET_LEN &&
            vc_ch->pending_recv.len + bytes_recvd == MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)))
        {
            MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, vc_ch->pending_recv.cell);
            goto fn_exit;
        }
    
        mpi_errno = breakout_pkts (vc, vc_ch->pending_recv.cell, vc_ch->pending_recv.len + bytes_recvd);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }
    else if (!MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
    {
        /* receive next packets into new cell */

        MPID_nem_queue_dequeue (MPID_nem_newtcp_module_free_queue, &v_cell);
        cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            
        CHECK_EINTR (bytes_recvd, read (vc->ch.fd, MPID_NEM_CELL_TO_PACKET (cell), MPID_NEM_MAX_PACKET_LEN));
        if (bytes_recvd == -1)
        {
            if (errno == EAGAIN)
                goto fn_exit;
            else
                MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
        }
        
        /* fast path: single packet case */
        if (vc_ch->pending_recv.len + bytes_recvd >= MPID_NEM_MIN_PACKET_LEN &&
            vc_ch->pending_recv.len + bytes_recvd == MPID_NEM_PACKET_LEN (MPID_NEM_CELL_TO_PACKET (vc_ch->pending_recv.cell)))
        {
            MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, vc_ch->pending_recv.cell);
            goto fn_exit;
        }
    
        mpi_errno = breakout_pkts (vc, cell, bytes_recvd);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);

        goto fn_exit;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPID_nem_newtcp_module_conn_est -- this function is called when the
   connection is finally extablished to send any pending sends */
#undef FUNCNAME
#define FUNCNAME recv_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_conn_est (struct pollfd *pfd, sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPID_nem_newtcp_module_send_queue (sc->vc);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_fail:    
 fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = MPID_nem_newtcp_module_send_list.head; vc; vc = vc->ch.newtcp_sendl_next)
    {
        mpi_errno = MPID_nem_newtcp_module_send_queue (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME recv_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int recv_progress()
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;
    MPIDI_VC_t *vc;
    MPID_nem_cell_ptr_t v_cell;
    MPID_nem_cell_t *cell; /* non-volatile cell */

    /* Copy any packets from overflow buf into cells first */
    if (recv_overflow_buf.start)
    {
        while (!MPID_nem_queue_empty (MPID_nem_newtcp_module_free_queue))
        {
            MPID_nem_pkt_t *pkt;
            int len;

            pkt = (MPID_nem_pkt_t *)recv_overflow_buf.start;
            len = (recv_overflow_buf.len < MPID_NEM_MIN_PACKET_LEN ||
                   recv_overflow_buf.len < MPID_NEM_PACKET_LEN (pkt))
                ? recv_overflow_buf.len : MPID_NEM_PACKET_LEN (pkt);

            /* allocate a new cell and copy the packet (or fragment) into it */
            MPID_nem_queue_dequeue (MPID_nem_newtcp_module_free_queue, &v_cell);
            cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            MPID_NEM_MEMCPY (&cell->pkt, pkt, len);
           
            if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (pkt))
            {
                /* this was just a packet fragment, attach the cell to the vc to be filled in later */
                MPIU_Assert (vc->ch.pending_recv.cell == NULL);
                recv_overflow_buf.vc->ch.pending_recv.cell = cell;
                recv_overflow_buf.vc->ch.pending_recv.end = (char *)(&cell->pkt) + len;
                recv_overflow_buf.vc->ch.pending_recv.len = len;

                /* there are no more packets in the overflow buffer */
                recv_overflow_buf.start = NULL;
                break;
            }

            /* update overflow buffer pointers */
            recv_overflow_buf.start += len;
            recv_overflow_buf.len -= len;
            
            if (recv_overflow_buf.len == 0)
            {
                /* there are no more packets in the overflow buffer */
                recv_overflow_buf.start = NULL;
                break;
            }
        }
    }
    
    mpi_errno = MPID_nem_newtcp_module_connpoll();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);


 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = send_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = recv_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

