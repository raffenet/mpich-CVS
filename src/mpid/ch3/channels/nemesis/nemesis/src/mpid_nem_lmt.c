/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

#define set_request_info(rreq_, pkt_, msg_type_)		\
{								\
    (rreq_)->status.MPI_SOURCE = (pkt_)->match.rank;		\
    (rreq_)->status.MPI_TAG = (pkt_)->match.tag;		\
    (rreq_)->status.count = (pkt_)->data_sz;			\
    (rreq_)->dev.sender_req_id = (pkt_)->sender_req_id;		\
    (rreq_)->dev.recv_data_sz = (pkt_)->data_sz;		\
    MPIDI_Request_set_seqnum((rreq_), (pkt_)->seqnum);		\
    MPIDI_Request_set_msg_type((rreq_), (msg_type_));		\
}

/* request completion actions */
static int do_cts(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete);
static int do_send(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete);
static int do_cookie(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete);

/* packet handlers */
static int pkt_RTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp);
static int pkt_CTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp);
static int pkt_DONE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp);
static int pkt_COOKIE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp);

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_pkthandler_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);

    /* Check that the array is large enough */
    if (arraySize <= MPIDI_NEM_PKT_LMT_COOKIE) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_INTERN, "**ch3|pktarraytoosmall");
    }

    pktArray[MPIDI_NEM_PKT_LMT_RTS] = pkt_RTS_handler;
    pktArray[MPIDI_NEM_PKT_LMT_CTS] = pkt_CTS_handler;
    pktArray[MPIDI_NEM_PKT_LMT_DONE] = pkt_DONE_handler;
    pktArray[MPIDI_NEM_PKT_LMT_COOKIE] = pkt_COOKIE_handler;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_PKTHANDLER_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPID_nem_lmt_RndvSend - Send a request to perform a rendezvous send */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_RndvSend
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_RndvSend(MPID_Request **sreq_p, const void * buf, int count, MPI_Datatype datatype, int dt_contig, int data_sz, 
                          MPI_Aint dt_true_lb, int rank, int tag, MPID_Comm * comm, int context_offset)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = &upkt.lmt_rts;
    MPIDI_VC_t *vc;
    MPID_Request *rts_sreq;
    MPID_Request *sreq =*sreq_p;
    MPID_IOV iov[2];
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_RNDVSEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_RNDVSEND);
  
    MPIDI_Comm_get_vc(comm, rank, &vc);

    /* if the lmt functions are not set, fall back to the default rendezvous code */
    if (vc->ch.lmt_initiate_lmt == NULL)
    {
        mpi_errno = MPIDI_CH3_RndvSend(sreq_p, buf, count, datatype, dt_contig, data_sz, dt_true_lb, rank, tag, comm, context_offset);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIU_DBG_MSG_D(CH3_OTHER,VERBOSE,
		   "sending lmt RTS, data_sz=" MPIDI_MSG_SZ_FMT, data_sz);
    sreq->partner_request = NULL;
    sreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;
	
    MPIDI_Pkt_init(rts_pkt, MPIDI_NEM_PKT_LMT_RTS);
    rts_pkt->match.rank	      = comm->rank;
    rts_pkt->match.tag	      = tag;
    rts_pkt->match.context_id = comm->context_id + context_offset;
    rts_pkt->sender_req_id    = sreq->handle;
    rts_pkt->data_sz	      = data_sz;

    MPIDI_VC_FAI_send_seqnum(vc, seqnum);
    MPIDI_Pkt_set_seqnum(rts_pkt, seqnum);
    MPIDI_Request_set_seqnum(sreq, seqnum);

    mpi_errno = vc->ch.lmt_initiate_lmt(vc, &upkt, sreq);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

/*     rts_pkt->cookie_len = cookie.MPID_IOV_LEN; */
    
/*     iov[0].MPID_IOV_BUF = rts_pkt; */
/*     iov[0].MPID_IOV_LEN = sizeof(*rts_pkt); */
/*     iov[1] = cookie; */

/*     MPIU_DBG_MSGPKT(vc, tag, rts_pkt->match.context_id, rank, data_sz, "Rndv"); */

/*     mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, (cookie.MPID_IOV_LEN) ? 2 : 1, &rts_sreq); */
/*     /\* --BEGIN ERROR HANDLING-- *\/ */
/*     if (mpi_errno != MPI_SUCCESS) */
/*     { */
/* 	MPIU_Object_set_ref(sreq, 0); */
/* 	MPIDI_CH3_Request_destroy(sreq); */
/* 	*sreq_p = NULL; */
/*         MPIU_ERR_SETFATALANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rtspkt"); */
/*     } */
/*     /\* --END ERROR HANDLING-- *\/ */
/*     if (rts_sreq != NULL) */
/*     { */
/* 	if (rts_sreq->status.MPI_ERROR != MPI_SUCCESS) */
/* 	{ */
/* 	    MPIU_Object_set_ref(sreq, 0); */
/* 	    MPIDI_CH3_Request_destroy(sreq); */
/* 	    *sreq_p = NULL; */
/* 	    mpi_errno = MPIR_Err_create_code(rts_sreq->status.MPI_ERROR, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0); */
/* 	    MPID_Request_release(rts_sreq); */
/* 	    goto fn_exit; */
/* 	} */
/* 	MPID_Request_release(rts_sreq); */
/*     } */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_RNDVSEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/*
 * This routine processes a rendezvous message once the message is matched.
 * It is used in mpid_recv and mpid_irecv.
 */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_RecvRndv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_RecvRndv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    int complete = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_RECVRNDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_RECVRNDV);

    /* if the lmt functions are not set, fall back to the default rendezvous code */
    if (vc->ch.lmt_initiate_lmt == NULL)
    {
        mpi_errno = MPIDI_CH3_RecvRndv(vc, rreq);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE, "lmt RTS in the request");

    mpi_errno = do_cts(vc, rreq, &complete);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(complete);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_RECVRNDV);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_RTS_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_RTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_PKT_RTS_HANDLER);
    MPID_Request * rreq;
    int found;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = &pkt->lmt_rts;
    MPIU_CHKPMEM_DECL(1);
        
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_RTS_HANDLER);

    MPIU_DBG_MSG_FMT(CH3_OTHER,VERBOSE,(MPIU_DBG_FDEST, "received LMT RTS pkt, sreq=0x%08x, rank=%d, tag=%d, context=%d, data_sz=%d",
                                        rts_pkt->sender_req_id, rts_pkt->match.rank, rts_pkt->match.tag, rts_pkt->match.context_id,
                                        rts_pkt->data_sz));

    rreq = MPIDI_CH3U_Recvq_FDP_or_AEU(&rts_pkt->match, &found);
    MPIU_ERR_CHKANDJUMP(rreq == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");

    set_request_info(rreq, rts_pkt, MPIDI_REQUEST_RNDV_MSG);

    rreq->ch.lmt_req_id = rts_pkt->sender_req_id;
    rreq->ch.lmt_data_sz = rts_pkt->data_sz;

    if (rts_pkt->cookie_len == 0)
    {
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;
        rreq->dev.iov_count = 0;
        *rreqp = NULL;

        if (found)
        {
            /* there's no cookie to receive, and we found a match, so handle the cts directly */
            int complete;
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"posted request found");
            mpi_errno = do_cts(vc, rreq, &complete);
            if (mpi_errno) MPIU_ERR_POP (mpi_errno);
            MPIU_Assert (complete);
        }
        else
        {
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");
            rreq->dev.OnDataAvail = 0;
            MPIDI_CH3_Progress_signal_completion();
        }
    }
    else
    {
        /* set for the cookie to be received into the tmp_cookie in the request */
        MPIU_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, char *, rts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = rts_pkt->cookie_len;
        
        rreq->dev.iov[0] = rreq->ch.lmt_tmp_cookie;
        rreq->dev.iov_count = 1;
        *rreqp = rreq;
        
        if (found)
        {
            rreq->dev.OnDataAvail = do_cts;
        }
        else
        {
            MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"unexpected request allocated");
            rreq->dev.OnDataAvail = 0;
            MPIDI_CH3_Progress_signal_completion();
        }
    }    
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_RTS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_CTS_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_CTS_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp)
{
    MPID_nem_pkt_lmt_cts_t * const cts_pkt = &pkt->lmt_cts;
    MPID_Request *sreq;
    MPID_Request *rts_sreq;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iov_n;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_PKT_CTS_HANDLER);
    
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_CTS_HANDLER);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"received rndv CTS pkt");

    MPID_Request_get_ptr(cts_pkt->sender_req_id, sreq);

    sreq->ch.lmt_req_id = cts_pkt->receiver_req_id;
    sreq->ch.lmt_data_sz = cts_pkt->data_sz;

    /* Release the RTS request if one exists.
       MPID_Request_fetch_and_clear_rts_sreq() needs to be atomic to
       prevent cancel send from cancelling the wrong (future) request.
       If MPID_Request_fetch_and_clear_rts_sreq() returns a NULL
       rts_sreq, then MPID_Cancel_send() is responsible for releasing
       the RTS request object. */
    MPIDI_Request_fetch_and_clear_rts_sreq(sreq, &rts_sreq);
    if (rts_sreq != NULL)
        MPID_Request_release(rts_sreq);

    if (cts_pkt->cookie_len != 0)
    {
        /* create a recv req and set up to receive the cookie into the sreq's tmp_cookie */
        MPID_Request *rreq;
        MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_fail);
        /* FIXME:  where does this request get freed? */

        MPIU_CHKPMEM_MALLOC(sreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, char *, cts_pkt->cookie_len, mpi_errno, "tmp cookie buf");
        sreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = cts_pkt->cookie_len;

        rreq->dev.iov[0] = sreq->ch.lmt_tmp_cookie;
        rreq->dev.iov_count = 1;
        rreq->ch.lmt_req = sreq;
        rreq->dev.OnDataAvail = do_send;
        *rreqp = rreq;
    }
    else
    {
        MPID_IOV cookie = {0,0};
        mpi_errno = vc->ch.lmt_start_send(vc, sreq, cookie);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
        *rreqp = NULL;
    }

    
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_CTS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_DONE_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_DONE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_done_t * const done_pkt = &pkt->lmt_done;
    MPID_Request *req;
    MPIDI_STATE_DECL(MPID_STATE_PKT_DONE_HANDLER);
    
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_DONE_HANDLER);

    MPID_Request_get_ptr(done_pkt->req_id, req);

    switch (MPIDI_Request_get_type(req))
    {
    case MPIDI_REQUEST_TYPE_RECV:
        mpi_errno = vc->ch.lmt_done_recv(vc, req);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        break;
    case MPIDI_REQUEST_TYPE_SEND:
        mpi_errno = vc->ch.lmt_done_send(vc, req);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        break;
    default:
        MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**intern", "**intern %s", "unexpected request type");
        break;
    }   

    *rreqp = NULL;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_DONE_HANDLER);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME pkt_COOKIE_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int pkt_COOKIE_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_cookie_t * const cookie_pkt = &pkt->lmt_cookie;
    MPID_Request *req;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_PKT_COOKIE_HANDLER);
    
    MPIDI_FUNC_ENTER(MPID_STATE_PKT_COOKIE_HANDLER);

    MPID_Request_get_ptr(cookie_pkt->req_id, req);

    if (cookie_pkt->cookie_len != 0)
    {
        /* create a recv req and set up to receive the cookie into the rreq's tmp_cookie */
        MPID_Request *rreq;

        MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_fail);
        /* FIXME:  where does this request get freed? */

        MPIU_CHKPMEM_MALLOC(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF, char *, cookie_pkt->cookie_len, mpi_errno, "tmp cookie buf");
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = cookie_pkt->cookie_len;

        rreq->dev.iov[0] = rreq->ch.lmt_tmp_cookie;
        rreq->dev.iov_count = 1;
        rreq->ch.lmt_req = req;
        rreq->dev.OnDataAvail = do_cookie;
        *rreqp = rreq;
    }
    else
    {
        MPID_IOV cookie = {0,0};

        mpi_errno = vc->ch.lmt_handle_cookie(vc, req, cookie);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        *rreqp = NULL;
    }

 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_PKT_COOKIE_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_cts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_cts(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_Request * cts_req;
    MPIDI_CH3_Pkt_t upkt;
    MPID_nem_pkt_lmt_cts_t * const cts_pkt = &upkt.lmt_cts;
    MPID_IOV s_cookie;
    MPIDI_STATE_DECL(MPID_STATE_DO_CTS);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_CTS);

    MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"posted request found");

    /* determine amount of data to be transfered and check for truncation */
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (rreq->ch.lmt_data_sz > data_sz)
    {
        rreq->status.MPI_ERROR = MPIU_ERR_SET2(mpi_errno, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d", rreq->ch.lmt_data_sz, data_sz);
        rreq->ch.lmt_data_sz = data_sz;
    }
    
    s_cookie = rreq->ch.lmt_tmp_cookie;

    mpi_errno = vc->ch.lmt_start_recv(vc, rreq, s_cookie);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* free cookie buffer allocated in RTS handler */
    if (rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN)
    {
        MPIU_Free(rreq->ch.lmt_tmp_cookie.MPID_IOV_BUF);
        rreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;
    }
    
/*     if (send_cts) */
/*     {             */
/*         MPID_IOV iov[2]; */

/*         MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"sending rndv CTS packet"); */
/*         MPIDI_Pkt_init(cts_pkt, MPIDI_NEM_PKT_LMT_CTS); */
/*         cts_pkt->sender_req_id = rreq->ch.lmt_req_id; */
/*         cts_pkt->receiver_req_id = rreq->handle; */
/*         cts_pkt->data_sz = rreq->ch.lmt_data_sz; */
/*         cts_pkt->cookie_len = r_cookie.MPID_IOV_LEN; */
                
/*         iov[0].MPID_IOV_BUF = cts_pkt; */
/*         iov[0].MPID_IOV_LEN = sizeof(*cts_pkt); */
/*         iov[1] = r_cookie; */
                
/*         mpi_errno = MPIDI_CH3_iStartMsgv(vc, iov, (r_cookie.MPID_IOV_LEN) ? 2 : 1, &cts_req); */
/*         MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|ctspkt"); */
/*         if (cts_req != NULL) */
/*             MPID_Request_release(cts_req); */
/*     } */
            
    *complete = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_CTS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME do_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_send(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV r_cookie;
    MPID_Request * const sreq = rreq->ch.lmt_req;
    MPIDI_STATE_DECL(MPID_STATE_DO_SEND);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_SEND);

    r_cookie = sreq->ch.lmt_tmp_cookie;

    mpi_errno = vc->ch.lmt_start_send(vc, sreq, r_cookie);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* free cookie buffer allocated in CTS handler */
    MPIU_Free(sreq->ch.lmt_tmp_cookie.MPID_IOV_BUF);
    sreq->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;

    *complete = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_SEND);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME do_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_cookie(MPIDI_VC_t *vc, MPID_Request *rreq, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV cookie;
    MPID_Request *req = rreq->ch.lmt_req;
    MPIDI_STATE_DECL(MPID_STATE_DO_COOKIE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_COOKIE);

    cookie = req->ch.lmt_tmp_cookie;

    mpi_errno = vc->ch.lmt_handle_cookie(vc, req, cookie);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    /* free cookie buffer allocated in COOKIE handler */
    MPIU_Free(req->ch.lmt_tmp_cookie.MPID_IOV_BUF);
    req->ch.lmt_tmp_cookie.MPID_IOV_LEN = 0;

    *complete = TRUE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_COOKIE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
