/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sctp_module_impl.h"

#define DO_PAPI3(x) /*x */

static MPID_nem_pkt_t conn_pkt;
static MPID_nem_pkt_t *conn_pkt_p = &conn_pkt;
static int conn_pkt_is_set = 0;


#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_sctp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;    
    int stream = 0; /* single stream implementation! need MPID_NEM_CELL_xxx
                     *  to get stream (and maybe context and maybe tag)
                     */
    int ret;
    int len;
    int errno_save;
    MPID_nem_pkt_t *pkt;
    
    pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */


    pkt->mpich2.datalen = datalen;
    pkt->mpich2.source  = MPID_nem_mem_region.rank;

    /* need to find out if we have to send the connection packet for this stream */
    
    if(vc->ch.stream_table[stream].have_sent_pg_id == HAVE_NOT_SENT_PG_ID) {        

        /* send connection pkt */

        if(!conn_pkt_is_set)
        {
            /* initialize connection pkt */

            len = (int) strlen(MPIDI_Process.my_pg->id);
            conn_pkt_p->mpich2.datalen = len + 1;
            conn_pkt_p->mpich2.source  = MPID_nem_mem_region.rank;
            conn_pkt_p->mpich2.dest = pkt->mpich2.dest;

            /* TODO make sure MPID_NEM_MPICH2_DATA_LEN > strlen(pg_id) */
            /*    FYI  as I type this, this is 64K bytes... */

            memset(MPID_NEM_PACKET_PAYLOAD(conn_pkt_p), 0, MPID_NEM_MPICH2_DATA_LEN);
            memcpy(MPID_NEM_PACKET_PAYLOAD(conn_pkt_p), MPIDI_Process.my_pg->id, len );

            conn_pkt_is_set++;                
        }

        ret = sctp_sendmsg(vc->ch.fd, conn_pkt_p, 
                           MPID_NEM_PACKET_LEN (conn_pkt_p),
                           (struct sockaddr *) &(vc->ch.to_address), sizeof(struct sockaddr_in),
                           0, 0, stream, 0, 0);        

        errno_save = errno;
        if(ret == -1) {
            /* TODO if unsuccessful, need to maintain a send Q with special case
             *   for connection packet
             */
            perror("sctp_sendmsg conn_pkt"); /* TODO check error codes */
            printf("errno_save is %d\n", errno_save);
        }

        
        vc->ch.stream_table[stream].have_sent_pg_id = HAVE_SENT_PG_ID;        
    }

    /* send cell */
    ret = sctp_sendmsg(vc->ch.fd, pkt, MPID_NEM_PACKET_LEN (pkt),
                       (struct sockaddr *)&(vc->ch.to_address), sizeof(struct sockaddr_in),
                       0, 0, stream, 0, 0);
    errno_save = errno;
    if(ret == -1) {
        /* TODO if unsuccessful, need to maintain a send Q */
        perror("sctp_sendmsg data"); /* TODO check error codes */
        printf("errno_save is %d\n", errno_save);
    } else {
        /* it was sent successfully so enqueue the cell back to the free_queue */
        MPID_nem_queue_enqueue (MPID_nem_process_free_queue, cell);
    }

    
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


int MPID_nem_sctp_module_send_progress()
{
    int mpi_errno = MPI_SUCCESS;

    /* TODO walk through the sendQ and try to send stuff */
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
    
}
