/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <errno.h>

#undef FUNCNAME
#define FUNCNAME send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = MPID_nem_send_list.head; vc; vc = vc->ch.newtcp_sendl_next)
    {
        mpi_errno = MPID_nem_newtcp_module_send_queue (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

typedef struct tmpbuf
{
    char *start;
    int len;
    struct tmpbuf *next;
    struct tmpbuf *prev;
    char buf[MPID_NEM_MAX_PACKET_LEN];
} tmpbuf_t;

static struct {tmp_buf_t *head, *tail;} tmpbuf_list;
    

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
    tmpbuf_t *tb;

    /* Copy any packets in tmpbufs into cells first */
    tb = L_HEAD (tmpbuf_list);
    if (tb)
    {
        while (!MPID_nem_queue_empty (MPID_nem_tcp_module_free_queue))
        {
            MPID_nem_pkt_t *pkt;
            int pktlen;

            pkt = tb->start;
        
            /* make sure we have at least one packet */
            if (tb->len < MPID_NEM_MIN_PACKET_LEN || tb->len < MPID_NEM_PACKET_LEN (pkt))
            {
                tb = tb->next;

                if (!tb)
                    break; /* no more tmpbufs */

                continue;
            }

            pktlen = MPID_NEM_PACKET_LEN (pkt);

            /* copy packet into a free cell */
            MPID_nem_queue_dequeue (MPID_nem_tcp_module_free_queue, v_cell);
            cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            MPID_NEM_MEMCPY (cell->pkt, pkt, pktlen);

            /* is this the last packet in this tmpbuf? */
            if (tb->len == pktlen)
            {
                tmpbuf_t *tt = tb;
                tb = tb->next;
                L_REMOVE (&tmpbuf_list, tt);

                if (!tb)
                    break; /* no more tmpbufs */

                continue;
            }

            /* point to the start of the next packet */
            tb->start += pktlen;
            tb->len -= pktlen;
        }
    }
    
    
    for (vc = first_ready_vc(); vc; vc = next_ready_vc())
    {
        MPIDI_CH3I_VC *vc_ch = &vc->ch;

        if (vc_ch->tmp_cell)
        {
            /* there is a partially received pkt in tmp_cell, continue receiving into it */
            cell = vc_ch->tmp_cell;
            do
            {
                bytes_recvd = read (vc->ch.fd, tmp_cell_start, tmp_cell_remaining);
            }
            while (bytes_recvd == -1 && errno = EINTR);
            if (bytes_recvd == -1)
            {
                if (errno == EAGAIN)
                    continue;
                else
                    MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
            }

            mpi_errno = breakout_pkts (vc, cell, bytes_recvd);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        }
        else if (!MPID_nem_queue_empty (MPID_nem_tcp_module_free_queue))
        {
            /* receive next packets into new cell */

            MPID_nem_queue_dequeue (MPID_nem_tcp_module_free_queue, v_cell);
            cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            
            do
            {
                bytes_recvd = read (vc->ch.fd, MPID_NEM_CELL_TO_PACKET (cell), MPID_NEM_MAX_PACKET_LEN);
            }
            while (bytes_recvd == -1 && errno = EINTR);
            if (bytes_recvd == -1)
            {
                if (errno == EAGAIN)
                    continue;
                else
                    MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
            }

            mpi_errno = breakout_pkts (vc, cell, bytes_recvd);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);

            continue;
        }
        
    }


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

