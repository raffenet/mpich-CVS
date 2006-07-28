/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <errno.h>

recv_overflow_buf_t MPID_nem_newtcp_module_recv_overflow_buf = {0};

#undef FUNCNAME
#define FUNCNAME send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_progress()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    
    for (vc = MPID_nem_tcp_module_send_list.head; vc; vc = vc->ch.newtcp_sendl_next)
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
    overflow_buf_t *tb;

    /* Copy any packets in overflow buf into cells first */
    if (MPID_nem_newtcp_module_recv_overflow_buf.start)
    {
        while (!MPID_nem_queue_empty (MPID_nem_tcp_module_free_queue))
        {
            MPID_nem_pkt_t *pkt;
            int len;

            pkt = MPID_nem_newtcp_module_recv_overflow_buf.start;
            len = (tb->len < MPID_NEM_MIN_PACKET_LEN || tb->len < MPID_NEM_PACKET_LEN (pkt)) ? tb->len : MPID_NEM_PACKET_LEN (pkt);

            /* allocate a new cell and copy the packet (or fragment) into it */
            MPID_nem_queue_dequeue (MPID_nem_tcp_module_free_queue, v_cell);
            cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
            MPID_NEM_MEMCPY (cell->pkt, pkt, len);
           
            if (len < MPID_NEM_MIN_PACKET_LEN || len < MPID_NEM_PACKET_LEN (pkt))
            {
                /* this was just a packet fragment, attach the cell to the vc to be filled in later */
                MPID_nem_newtcp_module_recv_overflow_buf.vc->ch.pending_recv.cell = cell;
                MPID_nem_newtcp_module_recv_overflow_buf.vc->ch.pending_recv.end = (char *)(cell->pkt) + len;
                MPID_nem_newtcp_module_recv_overflow_buf.vc->ch.pending_recv.len = len;

                /* there are no more packets in the overflow buffer */
                MPID_nem_newtcp_module_recv_overflow_buf.start = NULL;
                break;
            }

            /* update overflow buffer pointers */
            MPID_nem_newtcp_module_recv_overflow_buf.start += len;
            MPID_nem_newtcp_module_recv_overflow_buf.len -= len;
            
            if (MPID_nem_newtcp_module_recv_overflow_buf.len == 0)
            {
                /* there are no more packets in the overflow buffer */
                MPID_nem_newtcp_module_recv_overflow_buf.start = NULL;
                break;
            }
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

