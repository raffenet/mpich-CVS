/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <stdio.h>
#include <stdarg.h>

/* style: allow:vprintf:1 sig:0 */
/* style: allow:printf:2 sig:0 */

#undef MPIDI_dbg_printf
void MPIDI_dbg_printf(int level, char * func, char * fmt, ...)
{
    va_list list;
    
    MPIU_dbglog_printf("[%d] %s(): ", MPIR_Process.comm_world->rank, func);
    va_start(list, fmt);
    MPIU_dbglog_vprintf(fmt, list);
    va_end(list);
    MPIU_dbglog_printf("\n");
    fflush(stdout);
}

#undef MPIDI_err_printf
void MPIDI_err_printf(char * func, char * fmt, ...)
{
    va_list list;
    
    printf("[%d] ERROR - %s(): ", MPIR_Process.comm_world->rank, func);
    va_start(list, fmt);
    vprintf(fmt, list);
    va_end(list);
    printf("\n");
    fflush(stdout);
}

#ifdef MPICH_DBG_OUTPUT
void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt)
{
    MPIU_DBG_PRINTF(("MPIDI_CH3_Pkt_t:\n"));
    switch(pkt->type)
    {
    case MPIDI_CH3_PKT_EAGER_SEND:
#ifdef MPID_USE_SEQUENCE_NUMBERS
	MPIU_DBG_PRINTF((
	" type ......... EAGER_SEND\n"
	" sender_reqid . 0x%08x\n"
	" context_id ... %d\n"
	" data_sz ...... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" seqnum ....... %d\n",
	pkt->eager_send.sender_req_id,
	pkt->eager_send.match.context_id,
	pkt->eager_send.data_sz,
	pkt->eager_send.match.tag,
	pkt->eager_send.match.rank,
	pkt->eager_send.seqnum
	));
#else
	MPIU_DBG_PRINTF((
	" type ......... EAGER_SEND\n"
	" sender_reqid . 0x%08x\n"
	" context_id ... %d\n"
	" data_sz ...... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n",
	pkt->eager_send.sender_req_id,
	pkt->eager_send.match.context_id,
	pkt->eager_send.data_sz,
	pkt->eager_send.match.tag,
	pkt->eager_send.match.rank
	));
#endif
	break;
    case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
#ifdef MPID_USE_SEQUENCE_NUMBERS
	MPIU_DBG_PRINTF((
	" type ......... EAGER_SYNC_SEND\n"
	" context_id ... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" sender_reqid . 0x%08x\n"
	" data_sz ...... %d\n"
	" seqnum ....... %d\n",
	pkt->eager_sync_send.match.context_id,
	pkt->eager_sync_send.match.tag,
	pkt->eager_sync_send.match.rank,
	pkt->eager_sync_send.sender_req_id,
	pkt->eager_sync_send.data_sz,
	pkt->eager_sync_send.seqnum
	));
#else
	MPIU_DBG_PRINTF((
	" type ......... EAGER_SYNC_SEND\n"
	" context_id ... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" sender_reqid . 0x%08x\n"
	" data_sz ...... %d\n",
	pkt->eager_sync_send.match.context_id,
	pkt->eager_sync_send.match.tag,
	pkt->eager_sync_send.match.rank,
	pkt->eager_sync_send.sender_req_id,
	pkt->eager_sync_send.data_sz
	));
#endif
	break;
    case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
	MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_ACK\n sender_reqid . 0x%08x\n", pkt->eager_sync_ack.sender_req_id));
	break;
    case MPIDI_CH3_PKT_READY_SEND:
#ifdef MPID_USE_SEQUENCE_NUMBERS
	MPIU_DBG_PRINTF((
	" type ......... READY_SEND\n"
	" context_id ... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" sender_reqid . 0x%08x\n"
	" data_sz ...... %d\n"
	" seqnum ....... %d\n",
	pkt->ready_send.match.context_id,
	pkt->ready_send.match.tag,
	pkt->ready_send.match.rank,
	pkt->ready_send.sender_req_id,
	pkt->ready_send.data_sz,
	pkt->ready_send.seqnum
	));
#else
	MPIU_DBG_PRINTF((
	" type ......... READY_SEND\n"
	" context_id ... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" sender_reqid . 0x%08x\n"
	" data_sz ...... %d\n",
	pkt->ready_send.match.context_id,
	pkt->ready_send.match.tag,
	pkt->ready_send.match.rank,
	pkt->ready_send.sender_req_id,
	pkt->ready_send.data_sz
	));
#endif
	break;
    case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
#ifdef MPID_USE_SEQUENCE_NUMBERS
	MPIU_DBG_PRINTF((
	" type ......... REQ_TO_SEND\n"
	" sender_reqid . 0x%08x\n"
	" context_id ... %d\n"
	" data_sz ...... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" seqnum ....... %d\n",
	pkt->rndv_req_to_send.sender_req_id,
	pkt->rndv_req_to_send.match.context_id,
	pkt->rndv_req_to_send.data_sz,
	pkt->rndv_req_to_send.match.tag,
	pkt->rndv_req_to_send.match.rank,
	pkt->rndv_req_to_send.seqnum
	));
#else
	MPIU_DBG_PRINTF((
	" type ......... REQ_TO_SEND\n"
	" sender_reqid . 0x%08x\n"
	" context_id ... %d\n"
	" data_sz ...... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n",
	pkt->rndv_req_to_send.sender_req_id,
	pkt->rndv_req_to_send.match.context_id,
	pkt->rndv_req_to_send.data_sz,
	pkt->rndv_req_to_send.match.tag,
	pkt->rndv_req_to_send.match.rank
	));
#endif
	break;
    case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	MPIU_DBG_PRINTF((
	" type ......... CLR_TO_SEND\n"
	" sender_reqid . 0x%08x\n"
	" recvr_reqid .. 0x%08x\n",
	pkt->rndv_clr_to_send.sender_req_id,
	pkt->rndv_clr_to_send.receiver_req_id));
	break;
    case MPIDI_CH3_PKT_RNDV_SEND:
	MPIU_DBG_PRINTF((" type ......... RNDV_SEND\n recvr_reqid .. 0x%08x\n", pkt->rndv_send.receiver_req_id));
	break;
    case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	MPIU_DBG_PRINTF((
	" type ......... CANCEL_SEND\n"
	" context_id ... %d\n"
	" tag .......... %d\n"
	" rank ......... %d\n"
	" sender_reqid . 0x%08x\n",
	pkt->cancel_send_req.match.context_id,
	pkt->cancel_send_req.match.tag,
	pkt->cancel_send_req.match.rank,
	pkt->cancel_send_req.sender_req_id));
	break;
    case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
	MPIU_DBG_PRINTF((
	" type ......... CANCEL_SEND_RESP\n"
	" sender_reqid . 0x%08x\n"
	" ack .......... %d\n",
	pkt->cancel_send_resp.sender_req_id,
	pkt->cancel_send_resp.ack));
	break;
    case MPIDI_CH3_PKT_PUT:
	MPIU_DBG_PRINTF((" PUT\n"));
	break;
    case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
	MPIU_DBG_PRINTF((" FLOW_CNTRL_UPDATE\n"));
	break;
    default:
	MPIU_DBG_PRINTF((" unknown type %d\n", pkt->type));
	break;
    }
}
#endif
