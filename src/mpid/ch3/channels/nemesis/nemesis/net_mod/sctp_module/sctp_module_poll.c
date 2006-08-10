/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "sctp_module_impl.h"


#undef FUNCNAME
#define FUNCNAME send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_progress()
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =  MPID_nem_sctp_module_send_progress();

/*     MPIDI_VC_t *vc; */

    /* TODO need a send Q because can't assume success on first call to
     *   the net module's send */
    
/*     for (vc = MPID_nem_sctp_module_send_list.head; vc; vc = vc->ch.sctp_sendl_next) */
/*     { */
/*         mpi_errno = MPID_nem_sctp_module_send_queue (vc); */
/*         if (mpi_errno) MPIU_ERR_POP (mpi_errno); */
/*     } */

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
    struct sctp_sndrcvinfo sri;
    struct sockaddr from;
    socklen_t fromlen = sizeof(from);
    int msg_flags;
    MPID_nem_sctp_hash_entry *result;
    MPID_nem_sctp_hash_entry  lresult;
    
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;
    MPID_nem_cell_ptr_t v_cell;
    MPID_nem_cell_t *cell; /* non-volatile cell */


    /* can't control VC or stream you are reading from so read until you
     * would block or there are no more cells available in the free queue
     */
    while (!MPID_nem_queue_empty (MPID_nem_sctp_module_free_queue))
    {
        MPID_nem_queue_dequeue (MPID_nem_sctp_module_free_queue, &v_cell);
        cell = (MPID_nem_cell_t *)v_cell; /* cast away volatile */
        
        if(sctp_recvmsg(MPID_nem_sctp_onetomany_fd,
                        MPID_NEM_CELL_TO_PACKET (cell),
                        MPID_NEM_MAX_PACKET_LEN, &from,
                        &fromlen, &sri, &msg_flags)  == -1)
        {
            if(errno == EAGAIN) {
                /* don't want to waste the cell so enqueue it on the module free Q */
                MPID_nem_queue_enqueue (MPID_nem_sctp_module_free_queue, v_cell);
                break;
            } else {
                ; /* TODO handle real errors */
            }
        }
        else
        {
            /* received some data */
            
            if(msg_flags != MSG_EOR)
            {
                /* TODO partial delivery */;
            }
            else
            {
                /* standard case */
                    
                /* Look at association ID to see if this is
                 *  a new association.
                 */
                if((result = hash_find(MPID_nem_sctp_assocID_table, 
                                       (int4) sri.sinfo_assoc_id)) == NULL)
                {
                    /* new association  */
                    
                    result = &lresult;
                    result->assoc_id = sri.sinfo_assoc_id;

                    /* only one stream in initial design */

                    /* get PG from the connection packet */
                    MPIDI_PG_t * pg;
                    MPIDI_VC_t * vc;
                    char * data_ptr = (char *) MPID_NEM_CELL_TO_PACKET (cell);
                        
                    /* a new association so the first message must contain the pg_id, or
                     *  our protocol has been broken...
                     */

                    data_ptr += MPID_NEM_MPICH2_HEAD_LEN;
                        
                        
                    /* read pg_id from data_ptr and lookup VC using pg_id */
                    mpi_errno = MPIDI_PG_Find(data_ptr, &pg);
                    if (pg == NULL) {
                        MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
                                             "**pglookup",
                                             "**pglookup %s", data_ptr);
                    }
		
                    MPIDI_PG_Get_vc(pg, MPID_NEM_CELL_SOURCE(cell), &vc);
                    result->vc = vc;
                    vc->ch.state = MPID_NEM_VC_STATE_CONNECTED;
                    
                    /* Record that connection pkt has arrived.  */
                    vc->ch.stream_table[sri.sinfo_stream].have_recv_pg_id = HAVE_RECV_PG_ID;
                    
                    /* insert fully populated entry */
                    hash_insert(MPID_nem_sctp_assocID_table, result);

                    /* upper layer doesn't need to handle this since it is
                     *  only for SCTP net module to figure out the VC
                     */
                    MPID_nem_queue_enqueue (MPID_nem_sctp_module_free_queue, v_cell);
                }
                else
                {
                    /* seen this assocID before */

                    /* TODO In a multi-stream design, data may be a connection
                     *  pkt arriving on an existing association but a new stream.
                     */
                    
                    /* put cell on the process recv queue */
                    MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, v_cell);                    
                }                
            }
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_sctp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_sctp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

/*     mpi_errno = send_progress(); */
    mpi_errno = MPID_nem_sctp_module_send_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = recv_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

