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

/* FIXME: What are these routines for?  Who uses them?  Why are they different 
   from the src/util/dbg routines? */

#undef MPIDI_dbg_printf
void MPIDI_dbg_printf(int level, char * func, char * fmt, ...)
{
    /* FIXME: This "unreferenced_arg" is an example of a problem with the 
       API (unneeded level argument) or the code (failure to check the 
       level argument).  Inserting these "unreference_arg" macros erroneously
       suggests that the code is correct with this ununsed argument, and thus
       commits the grave harm of obscuring a real problem */
    MPIU_UNREFERENCED_ARG(level);
    MPID_Common_thread_lock();
    {
	va_list list;

	if (MPIR_Process.comm_world)
	{
	    MPIU_dbglog_printf("[%d] %s(): ", MPIR_Process.comm_world->rank, func);
	}
	else
	{
	    MPIU_dbglog_printf("[-1] %s(): ", func);
	}
	va_start(list, fmt);
	MPIU_dbglog_vprintf(fmt, list);
	va_end(list);
	MPIU_dbglog_printf("\n");
	fflush(stdout);
    }
    MPID_Common_thread_unlock();
}

#undef MPIDI_err_printf
void MPIDI_err_printf(char * func, char * fmt, ...)
{
    MPID_Common_thread_lock();
    {
	va_list list;

	if (MPIR_Process.comm_world)
	{
	    printf("[%d] ERROR - %s(): ", MPIR_Process.comm_world->rank, func);
	}
	else
	{
	    printf("[-1] ERROR - %s(): ", func);
	}
	va_start(list, fmt);
	vprintf(fmt, list);
	va_end(list);
	printf("\n");
	fflush(stdout);
    }
    MPID_Common_thread_unlock();
}

/* FIXME: It would be much better if the routine to print packets used
   routines defined by packet type, making it easier to modify the handling
   of packet types (and allowing channels to customize the printing
   of packets). For example, an array of function pointers, indexed by
   packet type, could be used.
   Also, these routines should not use MPIU_DBG_PRINTF, instead they should
   us a simple fprintf with a style allowance (so that the style checker
   won't flag the use as a possible problem).  */

#ifdef MPICH_DBG_OUTPUT
void MPIDI_DBG_Print_packet(MPIDI_CH3_Pkt_t *pkt)
{
    MPID_Common_thread_lock();
    {
	MPIU_DBG_PRINTF(("MPIDI_CH3_Pkt_t:\n"));
	switch(pkt->type)
	{
	    case MPIDI_CH3_PKT_EAGER_SEND:
		MPIU_DBG_PRINTF((" type ......... EAGER_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
		MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->eager_sync_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->eager_sync_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->eager_sync_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->eager_sync_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->eager_sync_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
		MPIU_DBG_PRINTF((" type ......... EAGER_SYNC_ACK\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->eager_sync_ack.sender_req_id));
		break;
	    case MPIDI_CH3_PKT_READY_SEND:
		MPIU_DBG_PRINTF((" type ......... READY_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->ready_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->ready_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->ready_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->ready_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->ready_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->ready_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
		MPIU_DBG_PRINTF((" type ......... REQ_TO_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->rndv_req_to_send.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->rndv_req_to_send.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->rndv_req_to_send.match.rank));
		MPIU_DBG_PRINTF((" data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF((" seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
		break;
	    case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
		MPIU_DBG_PRINTF((" type ......... CLR_TO_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
		MPIU_DBG_PRINTF((" recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
		break;
	    case MPIDI_CH3_PKT_RNDV_SEND:
		MPIU_DBG_PRINTF((" type ......... RNDV_SEND\n"));
		MPIU_DBG_PRINTF((" recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
		break;
	    case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
		MPIU_DBG_PRINTF((" type ......... CANCEL_SEND\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
		MPIU_DBG_PRINTF((" context_id ... %d\n", pkt->cancel_send_req.match.context_id));
		MPIU_DBG_PRINTF((" tag .......... %d\n", pkt->cancel_send_req.match.tag));
		MPIU_DBG_PRINTF((" rank ......... %d\n", pkt->cancel_send_req.match.rank));
		break;
	    case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
		MPIU_DBG_PRINTF((" type ......... CANCEL_SEND_RESP\n"));
		MPIU_DBG_PRINTF((" sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
		MPIU_DBG_PRINTF((" ack .......... %d\n", pkt->cancel_send_resp.ack));
		break;
	    case MPIDI_CH3_PKT_PUT:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_PUT\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->put.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->put.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->put.datatype));
		MPIU_DBG_PRINTF((" dataloop_size. 0x%08X\n", pkt->put.dataloop_size));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->put.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->put.source_win_handle));
		/*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->put.win_ptr));*/
		break;
	    case MPIDI_CH3_PKT_GET:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->get.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->get.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->get.datatype));
		MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->get.dataloop_size));
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request_handle));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->get.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->get.source_win_handle));
		/*
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get.request));
		MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->get.win_ptr));
		*/
		break;
	    case MPIDI_CH3_PKT_GET_RESP:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_GET_RESP\n"));
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request_handle));
		/*MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->get_resp.request));*/
		break;
	    case MPIDI_CH3_PKT_ACCUMULATE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_ACCUMULATE\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->accum.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->accum.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->accum.datatype));
		MPIU_DBG_PRINTF((" dataloop_size. %d\n", pkt->accum.dataloop_size));
		MPIU_DBG_PRINTF((" op ........... 0x%08X\n", pkt->accum.op));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->accum.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->accum.source_win_handle));
		/*MPIU_DBG_PRINTF((" win_ptr ...... 0x%08X\n", pkt->accum.win_ptr));*/
		break;
	    case MPIDI_CH3_PKT_LOCK:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK\n"));
		MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock.lock_type));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock.source_win_handle));
		break;
	    case MPIDI_CH3_PKT_LOCK_PUT_UNLOCK:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_PUT_UNLOCK\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_put_unlock.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_put_unlock.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_put_unlock.datatype));
		MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_put_unlock.lock_type));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_put_unlock.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_put_unlock.source_win_handle));
		break;
	    case MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_accum_unlock.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_accum_unlock.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_accum_unlock.datatype));
		MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_accum_unlock.lock_type));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_accum_unlock.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
		break;
	    case MPIDI_CH3_PKT_LOCK_GET_UNLOCK:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GET_UNLOCK\n"));
		MPIU_DBG_PRINTF((" addr ......... %p\n", pkt->lock_get_unlock.addr));
		MPIU_DBG_PRINTF((" count ........ %d\n", pkt->lock_get_unlock.count));
		MPIU_DBG_PRINTF((" datatype ..... 0x%08X\n", pkt->lock_get_unlock.datatype));
		MPIU_DBG_PRINTF((" lock_type .... %d\n", pkt->lock_get_unlock.lock_type));
		MPIU_DBG_PRINTF((" target ....... 0x%08X\n", pkt->lock_get_unlock.target_win_handle));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_get_unlock.source_win_handle));
		MPIU_DBG_PRINTF((" request ...... 0x%08X\n", pkt->lock_get_unlock.request_handle));
		break;
	    case MPIDI_CH3_PKT_PT_RMA_DONE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_PT_RMA_DONE\n"));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_accum_unlock.source_win_handle));
		break;
	    case MPIDI_CH3_PKT_LOCK_GRANTED:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_LOCK_GRANTED\n"));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->lock_granted.source_win_handle));
		break;
		/*
	    case MPIDI_CH3_PKT_SHARED_LOCK_OPS_DONE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_SHARED_LOCK_OPS_DONE\n"));
		MPIU_DBG_PRINTF((" source ....... 0x%08X\n", pkt->shared_lock_ops_done.source_win_handle));
		break;
		*/
	    case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
		MPIU_DBG_PRINTF((" FLOW_CNTRL_UPDATE\n"));
		break;
#ifdef MPIDI_CH3_CHANNEL_RNDV
	    case MPIDI_CH3_PKT_RTS_IOV:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_RTS_IOV\n"));
		MPIU_DBG_PRINTF((" sreq ......... 0x%08X\n", pkt->rts_iov.sreq));
		MPIU_DBG_PRINTF((" iov_len ...... %d\n", pkt->rts_iov.iov_len));
		break;
	    case MPIDI_CH3_PKT_CTS_IOV:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_CTS_IOV\n"));
		MPIU_DBG_PRINTF((" sreq ......... 0x%08X\n", pkt->cts_iov.sreq));
		MPIU_DBG_PRINTF((" rreq ......... 0x%08X\n", pkt->cts_iov.rreq));
		MPIU_DBG_PRINTF((" iov_len ...... %d\n", pkt->cts_iov.iov_len));
		break;
	    case MPIDI_CH3_PKT_RELOAD:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_RELOAD\n"));
		MPIU_DBG_PRINTF((" send_recv .... %d\n", pkt->reload.send_recv));
		MPIU_DBG_PRINTF((" sreq ......... 0x%08X\n", pkt->reload.sreq));
		MPIU_DBG_PRINTF((" rreq ......... 0x%08X\n", pkt->reload.rreq));
		break;
	    case MPIDI_CH3_PKT_IOV:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_IOV\n"));
		MPIU_DBG_PRINTF((" req .......... 0x%08X\n", pkt->iov.req));
		MPIU_DBG_PRINTF((" send_recv .... %d\n", pkt->iov.send_recv));
		MPIU_DBG_PRINTF((" iov_len ...... %d\n", pkt->iov.iov_len));
		break;
#endif
	    case MPIDI_CH3_PKT_CLOSE:
		MPIU_DBG_PRINTF((" type ......... MPIDI_CH3_PKT_CLOSE\n"));
		MPIU_DBG_PRINTF((" ack ......... %s\n", pkt->close.ack ? "TRUE" : "FALSE"));
		break;
	    
	    default:
		MPIU_DBG_PRINTF((" INVALID PACKET\n"));
		MPIU_DBG_PRINTF((" unknown type ... %d\n", pkt->type));
		MPIU_DBG_PRINTF(("  type .......... EAGER_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->eager_send.sender_req_id));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->eager_send.match.context_id));
		MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->eager_send.data_sz));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->eager_send.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->eager_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->eager_send.seqnum));
#endif
		MPIU_DBG_PRINTF(("  type .......... REQ_TO_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_req_to_send.sender_req_id));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->rndv_req_to_send.match.context_id));
		MPIU_DBG_PRINTF(("   data_sz ...... %d\n", pkt->rndv_req_to_send.data_sz));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->rndv_req_to_send.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->rndv_req_to_send.match.rank));
#ifdef MPID_USE_SEQUENCE_NUMBERS
		MPIU_DBG_PRINTF(("   seqnum ....... %d\n", pkt->rndv_req_to_send.seqnum));
#endif
		MPIU_DBG_PRINTF(("  type .......... CLR_TO_SEND\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->rndv_clr_to_send.sender_req_id));
		MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_clr_to_send.receiver_req_id));
		MPIU_DBG_PRINTF(("  type .......... RNDV_SEND\n"));
		MPIU_DBG_PRINTF(("   recvr_reqid .. 0x%08X\n", pkt->rndv_send.receiver_req_id));
		MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND\n"));
		MPIU_DBG_PRINTF(("   context_id ... %d\n", pkt->cancel_send_req.match.context_id));
		MPIU_DBG_PRINTF(("   tag .......... %d\n", pkt->cancel_send_req.match.tag));
		MPIU_DBG_PRINTF(("   rank ......... %d\n", pkt->cancel_send_req.match.rank));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_req.sender_req_id));
		MPIU_DBG_PRINTF(("  type .......... CANCEL_SEND_RESP\n"));
		MPIU_DBG_PRINTF(("   sender_reqid . 0x%08X\n", pkt->cancel_send_resp.sender_req_id));
		MPIU_DBG_PRINTF(("   ack .......... %d\n", pkt->cancel_send_resp.ack));
		break;
	}
    }
    MPID_Common_thread_unlock();
}
#endif


const char * MPIDI_VC_GetStateString(int state)
{
    switch (state)
    {
	case MPIDI_VC_STATE_INACTIVE:
	    return "MPIDI_VC_STATE_INACTIVE";
	case MPIDI_VC_STATE_ACTIVE:
	    return "MPIDI_VC_STATE_ACTIVE";
	case MPIDI_VC_STATE_LOCAL_CLOSE:
	    return "MPIDI_VC_STATE_LOCAL_CLOSE";
	case MPIDI_VC_STATE_REMOTE_CLOSE:
	    return "MPIDI_VC_STATE_REMOTE_CLOSE";
	case MPIDI_VC_STATE_CLOSE_ACKED:
	    return "MPIDI_VC_STATE_CLOSE_ACKED";
	default:
	    return "unknown";
    }
}

/* This routine is not thread safe and should only be used while
   debugging.  It is used to encode a brief description of a message
   packet into a string to make it easy to include in the message log
   output (with no newlines to simplify extracting info from the log file) 
*/
const char *MPIDI_Pkt_GetDescString( MPIDI_CH3_Pkt_t *pkt ) 
{
    static char pktmsg[256];

    /* For data messages, the string (...) is (context,tag,rank,size) */
    switch(pkt->type) {
    case MPIDI_CH3_PKT_EAGER_SEND:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "EAGER_SEND - (%d,%d,%d,%d)", 
		       pkt->eager_send.match.context_id,
		       pkt->eager_send.match.tag, 
		       pkt->eager_send.match.rank, 
		       pkt->eager_send.data_sz );
	break;
    case MPIDI_CH3_PKT_EAGER_SYNC_SEND:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "EAGER_SYNC_SEND - (%d,%d,%d,%d) req=%d", 
		       pkt->eager_sync_send.match.context_id,
		       pkt->eager_sync_send.match.tag, 
		       pkt->eager_sync_send.match.rank, 
		       pkt->eager_sync_send.data_sz,
		       pkt->eager_sync_send.sender_req_id );
		break;
    case MPIDI_CH3_PKT_EAGER_SYNC_ACK:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "EAGER_SYNC_ACK - req=%d", 
		       pkt->eager_sync_ack.sender_req_id );
	break;
    case MPIDI_CH3_PKT_READY_SEND:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "READY_SEND - (%d,%d,%d,%d)", 
		       pkt->ready_send.match.context_id,
		       pkt->ready_send.match.tag, 
		       pkt->ready_send.match.rank, 
		       pkt->ready_send.data_sz );
	break;
    case MPIDI_CH3_PKT_RNDV_REQ_TO_SEND:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "RNDV_REQ_TO_SEND - (%d,%d,%d,%d) req=%d", 
		       pkt->rndv_req_to_send.match.context_id,
		       pkt->rndv_req_to_send.match.tag, 
		       pkt->rndv_req_to_send.match.rank, 
		       pkt->rndv_req_to_send.data_sz,
		       pkt->rndv_req_to_send.sender_req_id );
	break;
    case MPIDI_CH3_PKT_RNDV_CLR_TO_SEND:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "RNDV_CLRTO_SEND - req=%d, recv req=%d", 
		       pkt->rndv_clr_to_send.sender_req_id,
		       pkt->rndv_clr_to_send.receiver_req_id );
		break;
    case MPIDI_CH3_PKT_RNDV_SEND:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "RNDV_SEND - recv req=%d", 
		       pkt->rndv_send.receiver_req_id );
	break;
    case MPIDI_CH3_PKT_CANCEL_SEND_REQ:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "CANCEL_SEND_REQ - req=%d", 
		       pkt->cancel_send_req.sender_req_id );
	break;
    case MPIDI_CH3_PKT_CANCEL_SEND_RESP:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "CANCEL_SEND_RESP - req=%d ack=%d", 
		       pkt->cancel_send_resp.sender_req_id, 
		       pkt->cancel_send_resp.ack );
	break;
    case MPIDI_CH3_PKT_PUT:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "PUT - (%p,%d,0x%08X)", 
		       pkt->put.addr, 
		       pkt->put.count,
		       pkt->put.target_win_handle );
		break;
    case MPIDI_CH3_PKT_GET:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "GET - (%p,%d,0x%08X) req=%d", 
		       pkt->get.addr, 
		       pkt->get.count,
		       pkt->get.target_win_handle,
		       pkt->get.request_handle );
	break;
    case MPIDI_CH3_PKT_GET_RESP:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "GET_RESP - req=%d", 
		       pkt->get_resp.request_handle );
	break;
    case MPIDI_CH3_PKT_ACCUMULATE:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "ACCUMULATE - (%p,%d,0x%08X)", 
		       pkt->accum.addr,
		       pkt->accum.count, 
		       pkt->accum.target_win_handle );
	break;
    case MPIDI_CH3_PKT_LOCK:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "LOCK - %d", 
		       pkt->lock.target_win_handle );
	break;
    case MPIDI_CH3_PKT_LOCK_PUT_UNLOCK:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "PUT_UNLOCK - (%p,%d,0x%08X)", 
		       pkt->lock_put_unlock.addr,
		       pkt->lock_put_unlock.count,
		       pkt->lock_put_unlock.target_win_handle );
	break;
    case MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "LOCK_ACCUM_UNLOCK - (%p,%d,0x%08X)", 
		       pkt->lock_accum_unlock.addr,
		       pkt->lock_accum_unlock.count,
		       pkt->lock_accum_unlock.target_win_handle );
	break;
    case MPIDI_CH3_PKT_LOCK_GET_UNLOCK:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "LOCK_GET_UNLOCK - (%p,%d,0x%08X) req=%d", 
		       pkt->lock_get_unlock.addr,
		       pkt->lock_get_unlock.count,
		       pkt->lock_get_unlock.target_win_handle, 
		       pkt->lock_get_unlock.request_handle );
	break;
    case MPIDI_CH3_PKT_PT_RMA_DONE:
	/* There is no rma_done packet type */
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "RMA_DONE - 0x%08X", 
		       pkt->lock_accum_unlock.source_win_handle );
	break;
    case MPIDI_CH3_PKT_LOCK_GRANTED:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "LOCK_GRANTED - 0x%08X", 
		       pkt->lock_granted.source_win_handle );
		break;
    case MPIDI_CH3_PKT_FLOW_CNTL_UPDATE:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "FLOW_CNTL_UPDATE" );
	break;
#ifdef MPIDI_CH3_CHANNEL_RNDV
    case MPIDI_CH3_PKT_RTS_IOV:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "RTS_IOV - sreq=0x%08X, len=%d", 
		       pkt->rts_iov.sreq, 
		       pkt->rts_iov.iov_len );
	break;
    case MPIDI_CH3_PKT_CTS_IOV:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "CTS_IOV - sreq=0x%08X, rreq=0x%08X, len=%d", 
		       pkt->cts_iov.sreq,
		       pkt->cts_iov.rreq,
		       pkt->cts_iov.iov_len );
	break;
    case MPIDI_CH3_PKT_RELOAD:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "RELOAD  - sreq=0x%08X, rreq=0x%08X, sendrecv=%d", 
		       pkt->reload.sreq,
		       pkt->reload.rreq,
		       pkt->reload.send_recv );
	break;
    case MPIDI_CH3_PKT_IOV:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "IOV - req=%d, sendrecv=%d, len=%d",
		       pkt->iov.req,
		       pkt->iov.send_recv,
		       pkt->iov.iov_len );
	break;
#endif
    case MPIDI_CH3_PKT_CLOSE:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "CLOSE ack=%d", 
		       pkt->close.ack );
	break;
	    
    default:
	MPIU_Snprintf( pktmsg, sizeof(pktmsg), 
		       "INVALID PACKET type=%d", pkt->type );
	break;
    }

    return pktmsg;
}
