/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* myct: added for SCTP */
#include "sctp_common.h"


volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    volatile int MPIDI_CH3I_progress_blocked = FALSE;
    volatile int MPIDI_CH3I_progress_wakeup_signalled = FALSE;

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
MPID_Thread_cond_t MPIDI_CH3I_progress_completion_cond;
#   endif
    static int MPIDI_CH3I_Progress_delay(unsigned int completion_count);
    static int MPIDI_CH3I_Progress_continue(unsigned int completion_count);
#endif

static inline void connection_free(MPIDI_VC_t * vc, int stream);
static inline int stream_post_sendq_req(MPIDI_VC_t * vc, int stream);
static inline int connection_post_send_pkt(MPIDI_VC_t * vc, int stream);
static inline int connection_post_recv_pkt(MPIDI_VC_t * vc, int stream);
static inline void connection_post_send_pkt_and_pgid(MPIDI_VC_t * vc, int stream);
/* static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb); */
static inline int adjust_posted_iov(SCTP_IOV* post_ptr, MPIU_Size_t nb);

/* receive buffer since we never know what stream or association data will arrive */
/* static char sctp_advbuf[READ_AMOUNT]; */   /* i.e. larger than the socket receive buffer */
/* sri contains association ID and stream number, etc. */
static sctp_rcvinfo sctp_sri;

/* added for SCTP.  May be moved to mpid/common later (if used elsewhere) */
static int MPIDU_Sctpi_socket_bufsz = 0;

GLB_SendQ_Head Global_SendQ;

BufferNode_t FirstBufferNode;

static int MPIDU_Sctp_init(void);
/* static int MPIDU_Sctp_wait(int fd, int timeout, MPIDU_Sctp_event_t * event); */
/* static int MPIDI_CH3I_Progress_handle_sctp_event(MPIDU_Sctp_event_t * event); */
static int MPIDU_Sctp_post_close(MPIDI_VC_t * vc);
static int MPIDU_Sctp_finalize(void);

inline static int read_from_advbuf_and_adjust(MPIDI_VC_t* vc, int stream, int amount,
					      char* src, MPID_Request* rreq);

int MPIDI_CH3I_listener_port;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_test(void)
{
    MPIDU_Sctp_event_t event;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);

#   if (MPICH_THREAD_LEVEL >= MPI_THREAD_MULTIPLE)
    {
	if (MPIDI_CH3I_progress_blocked == TRUE) 
	{
	    /*
	     * Another thread is already blocking in the progress engine.  We are not going to block waiting for progress, so we
	     * simply return.  It might make sense to yield before * returning, giving the PE thread a change to make progress.
	     *
	     * MT: Another thread is already blocking in poll.  Right now, calls to the progress routines are effectively
	     * serialized by the device.  The only way another thread may enter this function is if MPIDU_Sock_wait() blocks.  If
	     * this changes, a flag other than MPIDI_CH3I_Progress_blocked may be required to determine if another thread is in
	     * the progress engine.
	     */
	    
	    goto fn_exit;
	}
    }
#   endif
    
    mpi_errno = MPIDU_Sctp_wait(MPIDI_CH3I_onetomany_fd , 0, &event);

    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(&event);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|sock|handle_sock_event");
	}
    }
    else if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDU_SOCK_ERR_TIMEOUT)
    {
	mpi_errno = MPI_SUCCESS;
	goto fn_exit;
    }
    else {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**progress_sock_wait");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Progress_test() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_wait(MPID_Progress_state * progress_state)
{
    MPIDU_Sctp_event_t event;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_WAIT);
    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));

    /*
     * MT: the following code will be needed if progress can occur between MPIDI_CH3_Progress_start() and
     * MPIDI_CH3_Progress_wait(), or iterations of MPIDI_CH3_Progress_wait().
     *
     * This is presently not possible, and thus the code is commented out.
     */
#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
    {
	if (progress_state->ch.completion_count != MPIDI_CH3I_progress_completion_count)
	{
	    goto fn_exit;
	}
    }
#   endif
	
#   if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    {
	if (MPIDI_CH3I_progress_blocked == TRUE) 
	{
	    /*
	     * Another thread is already blocking in the progress engine.
	     *
	     * MT: Another thread is already blocking in poll.  Right now, calls to MPIDI_CH3_Progress_wait() are effectively
	     * serialized by the device.  The only way another thread may enter this function is if MPIDU_Sock_wait() blocks.  If
	     * this changes, a flag other than MPIDI_CH3I_Progress_blocked may be required to determine if another thread is in
	     * the progress engine.
	     */
	    MPIDI_CH3I_Progress_delay(MPIDI_CH3I_progress_completion_count);
		
	    goto fn_exit;
	}
    }
#   endif
    
    do
    {
#       if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
	{
	    MPIDI_CH3I_progress_blocked = TRUE;
	}
#	endif
	
	mpi_errno = MPIDU_Sctp_wait(MPIDI_CH3I_onetomany_fd, MPIDU_SCTP_INFINITE_TIME, &event);

#       if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
	{
	    MPIDI_CH3I_progress_blocked = FALSE;
	    MPIDI_CH3I_progress_wakeup_signalled = FALSE;
	}
#	endif

	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Assert(MPIR_ERR_GET_CLASS(mpi_errno) != MPIDU_SOCK_ERR_TIMEOUT);
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**progress_sock_wait");
	    goto fn_fail;
	}
	/* --END ERROR HANDLING-- */

	mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(&event);
	if (mpi_errno != MPI_SUCCESS) {
	    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
				"**ch3|sock|handle_sock_event");
	}
    }
    while (progress_state->ch.completion_count == MPIDI_CH3I_progress_completion_count);

    /*
     * We could continue to call MPIU_Sock_wait in a non-blocking fashion 
     * and process any other events; however, this would not
     * give the application a chance to post new receives, and thus could 
     * result in an increased number of unexpected messages
     * that would need to be buffered.
     */
    
#   if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    {
	/*
	 * Awaken any threads which are waiting for the progress that just occurred
	 */
	MPIDI_CH3I_Progress_continue(MPIDI_CH3I_progress_completion_count);
    }
#   endif
    
 fn_exit:
    /*
     * Reset the progress state so it is fresh for the next iteration
     */
    progress_state->ch.completion_count = MPIDI_CH3I_progress_completion_count;
    
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_WAIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Progress_wait() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to CONN_STATE_CLOSING");

    mpi_errno = MPIDU_Sctp_post_close(vc);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3_Connection_terminate() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init(int pg_size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
	MPID_Thread_cond_create(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif

    MPIDI_CH3I_onetomany_fd = -1;
    MPIDI_CH3I_dynamic_tmp_vc = NULL;
    MPIDI_CH3I_dynamic_tmp_fd = -1;
    
    
    mpi_errno = MPIDU_Sctp_init();
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    /* initialize eventq */
    eventq_head = NULL;
    eventq_tail = NULL;
    
    /* initialize hash table */
    MPIDI_CH3I_assocID_table = hash_init(pg_size, sizeof(MPIDI_CH3I_Hash_entry), INT4_MAX, 0);

    /* initialize global sendQ */
    Global_SendQ_init();

  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MIPDI_CH3I_Progress_init() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    
    /* added code to close one-to-many socket here to simplify MPIDI_CH3_Channel_close */
    if (close(MPIDI_CH3I_onetomany_fd) == -1) {    
        mpi_errno--;/*FIXME need new error code */
        goto fn_fail;
    }    

    /* no concept of listener for SCTP  */
    
    /* Shut down the listener */
/*     mpi_errno = MPIDU_Sock_post_close(MPIDI_CH3I_listener_conn->sock); */
/*     if (mpi_errno != MPI_SUCCESS) { */
/* 	MPIU_ERR_POP(mpi_errno); */
/*     } */
    
/*     MPID_Progress_start(&progress_state); */
/*     while(MPIDI_CH3I_listener_conn != NULL) */
/*     { */
/* 	mpi_errno = MPID_Progress_wait(&progress_state); */
	
/*     } */
/*     MPID_Progress_end(&progress_state); */
    
    /* FIXME: Cleanly shutdown other socks and free connection structures. (close protocol?) */


    /*
     * MT: in a multi-threaded environment, finalize() should signal any 
     * thread(s) blocking on MPIDU_Sock_wait() and wait for
     * those * threads to complete before destroying the progress engine data structures.
     */

/*     MPIDU_Sock_destroy_set(MPIDI_CH3I_sock_set); */
/*     MPIDU_Sock_finalize(); */

    /* brad : destroy eventq */
    MPIDU_Sctp_finalize();

    /* myct: finalize hash table */
    hash_free(MPIDI_CH3I_assocID_table);

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
	MPID_Thread_cond_destroy(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif
    
  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3I_Progress_finalize() */


#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_wakeup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_Progress_wakeup(void)
{
    MPIDU_Sock_wakeup(MPIDI_CH3I_sock_set);
}
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Get_business_card(char *value, int length)
{
    return my_MPIDI_CH3U_Get_business_card_sock(&value, &length);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_handle_sctp_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* static */ int MPIDI_CH3I_Progress_handle_sctp_event(MPIDU_Sctp_event_t * event)
{
    int complete;
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    
    MPIDI_CH3I_Hash_entry * result;
    MPIDI_CH3I_Hash_entry  lresult;    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SCTP_EVENT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SCTP_EVENT);

    switch (event->op_type)
    {
	case MPIDU_SCTP_OP_READ:
	{
            MPIDI_VC_t * vc;
            int stream = event-> sri.sinfo_stream;
	    int num_bytes = event-> num_bytes;
            /* see if this is a new association */

            result = hash_find(MPIDI_CH3I_assocID_table, (int4) event->sri.sinfo_assoc_id);

            if((result == NULL) && (MPIDI_CH3I_onetomany_fd == event->fd) )
            {
                /* new association */
                
                result = &lresult;
                result->assoc_id = event->sri.sinfo_assoc_id;
                
                /* check flags for MSG_EOR (i.e. a complete message) */
		MPIU_Assert(event->user_value & MSG_EOR);
                
		MPIDI_PG_t * pg;
		MPIDI_CH3_Pkt_t * pkt = event->user_ptr;
		char * data_ptr = event->user_ptr;
              
                if(pkt->type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
                {
                    /*  a connect is being initiated by the other side, and we've never seen
                     *   this association.
                     */
                    
                    /* so that this is done instantaneously, change the type to ACCEPT and call
                     *  this recursively (this way we can put the accept code in its own place)
                     */
                    event->op_type = MPIDU_SCTP_OP_ACCEPT;
                    mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(event);

                    goto fn_exit;
                }

		/* a new association so the first message must contain the pg_id, or
		 *  our protocol has been broken...
		 */
		MPIU_Assert(pkt->type == MPIDI_CH3I_PKT_SC_OPEN_REQ);

                                
		/* read pg_id from event->user_ptr and lookup VC using pg_id */
		data_ptr += sizeof(MPIDI_CH3_Pkt_t);   /* assumed to be NULL terminated */
		mpi_errno = MPIDI_PG_Find(data_ptr, &pg);
		if (pg == NULL) {                    
		    MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
					 "**pglookup",
					 "**pglookup %s", data_ptr);
		}

                /* get VC from an existing PG */
		MPIDI_PG_Get_vc(pg, pkt->sc_open_req.pg_rank, &vc);
		MPIU_Assert(vc->pg_rank == pkt->sc_open_req.pg_rank);
		result->vc = vc;
                vc->ch.sinfo_assoc_id = result->assoc_id;
                    
		/* will insert fully populated hash_entry below */
                hash_insert(MPIDI_CH3I_assocID_table, result);
            }
            else
            {
                /* we have seen this association before */
                MPIDI_VC_t * vc;
                MPIDI_CH3_Pkt_t * pkt =  event->user_ptr;

                if(MPIDI_CH3I_dynamic_tmp_fd == event->fd)
                {
                    if(MPIDI_CH3I_dynamic_tmp_vc == NULL &&
                       pkt-> type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
                    {
                        /*  the other side is ack'ing our connect.  */

                        /* TODO assert received on control stream? */
                        
                        /*  process this instantaneously. Change the type to ACCEPT
                         *  and call this recursively (this way we can put the
                         *  accept code in its own place)
                         */
                        event->op_type = MPIDU_SCTP_OP_ACCEPT;
                        mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(event);

                        goto fn_exit;
                    }
                    else
                    {
                        /* FIXME is it possible for tmp_vc to be non-NULL and pkt
                         *         to be an accept pkt?
                         */
                        
                        /* in the middle of an MPI_Connect/Accept, so use tmp_vc */
                        vc = MPIDI_CH3I_dynamic_tmp_vc;
                    }
                }                
                else
                {
                    /* business as usual */
                    vc = result-> vc;
                }

                char* adv_buffer = (char*) event-> user_ptr;

		/* VC cannot be NULL */
		MPIU_Assert(vc != NULL);    
                    
		/* whatever is in the adv_buffer is not control packet */
		MPID_Request* rreq = RECV_ACTIVE(vc, stream);
		MPID_IOV* iovp;
			
		int actual_bytes_read = event-> num_bytes;

		// if recv active is NULL, we are waiting for a pkt/(pkt+msg)
		if(rreq == NULL) {
		    /* FIXME brad : what if not a full pkt (!MSG_EOR)? */
		    
		    MPIU_Assert(num_bytes >= sizeof(MPIDI_CH3_Pkt_t));
			
		    /* if it's connection PKT, discard */
		    if(pkt-> type == MPIDI_CH3I_PKT_SC_OPEN_REQ) 
			break;

                    /* need to do ACCEPT stuff if this is an initial pkt */
                    if(pkt-> type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
                    {
                        /* I don't know if this can happen here... */
                        event->op_type = MPIDU_SCTP_OP_ACCEPT;
                        mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(event);
                        goto fn_exit;
                    }
		    
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(vc, pkt, &rreq);
		    
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
		    
		    /*  rreq can equal NULL (e.g. in close protocol), so need to
		     *    break cases apart to avoid dereferencing a NULL ptr
		     */
		    if(rreq) {	
			/* minus the size of envelope */
			actual_bytes_read -= sizeof(*pkt); 
			adv_buffer += sizeof(*pkt);
		    }
		} 

		mpi_errno = read_from_advbuf_and_adjust(vc, stream, actual_bytes_read,
							adv_buffer, rreq);
		
            }
	    
	    break;
	}
    case MPIDU_SCTP_OP_WRITE:
	{
	    MPID_IOV* iovp;
            MPIU_Size_t nb;
	    int iov_cnt;
	    SCTP_IOV* post_ptr = NULL;

            /* retrieve the information from the event */
	    int event_fd = event->fd;
	    MPIDI_VC_t* vc = (MPIDI_VC_t*) event-> user_ptr;  /* points to VC for writes */
	    int stream_no = event->user_value;

	    MPID_Request* nextReq = NULL;
	    
	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS) {
		mpi_errno = event->error;
		MPIU_ERR_POP(mpi_errno);
	    }
	    /* --END ERROR HANDLING-- */
	    
	    MPIU_Assert(SEND_ACTIVE(vc, stream_no) != NULL);
	    
	    MPID_Request* sreq = SEND_ACTIVE(vc, stream_no);
	    
	    mpi_errno = MPIDI_CH3U_Handle_send_req(vc, sreq, &complete);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
	    
	    if (complete){
		
		if(MPIDI_CH3I_VC_STATE_CONNECTING == SEND_CONNECTED(vc, stream_no)) {
		    SEND_CONNECTED(vc, stream_no) = MPIDI_CH3I_VC_STATE_CONNECTED;
		    vc->ch.send_init_count++;

		    if(vc->ch.send_init_count >= CONNECT_THRESHOLD) {
			int i = 0;
			for(i = 0; i < MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT; i++) {
			    if(SEND_CONNECTED(vc, i) != MPIDI_CH3I_VC_STATE_CONNECTING) 
				SEND_CONNECTED(vc, i) = MPIDI_CH3I_VC_STATE_CONNECTED;
			}
			vc->ch.send_init_count = -1;
		    }
		}
		
		stream_post_sendq_req(vc, stream_no);

	    } else /* more data to send */ {
		printf("this part not tested yet\n");
			
		/* force to finish sending the send active one!! */
		for(;;){
		    iovp = sreq->dev.iov;  /* FIXME connection_iov? */
			
		    mpi_errno = MPIDU_Sctp_writev(vc, iovp, sreq->dev.iov_count,
                                                  stream_no, 0, &nb);
			
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS) {
			/* mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, */
			    /* 							     "**ch3|sock|immedwrite", "ch3|sock|immedwrite %p %p %p", */
			/* 							     sreq, conn, conn->vc); */
			goto fn_fail;
		    }
		    /* --END ERROR HANDLING-- */
			
		    /* MPIDI_DBG_PRINTF((55, FCNAME, "immediate writev, vc=0x%p, sreq=0x%08x, nb=%d", */
		    /* 					  conn->vc, sreq->handle, nb)); */
		    
		    if (nb > 0 && adjust_iov(&iovp, &sreq->dev.iov_count, nb)) {
			mpi_errno = MPIDI_CH3U_Handle_send_req(vc, sreq, &complete);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_POP(mpi_errno);
			}
			
			if (complete) {
			    /*  need to reset active  */
			    SEND_ACTIVE(vc, stream_no) = NULL;
                            
			    break;
			}
		    }
		    else
			{
			    MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", vc, sreq->handle));
			    /* put it back to sendQ */
			    /* the offset is wrong, needs to be changed */
			    MPIDU_Sctp_post_writev(vc, sreq, sreq->dev.iov_count,
                                                   NULL, stream_no);
			    
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS) {
				/* mpi_errno = MPIR_Err_create_code( */
				/* 				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postwrite", */
				/* 				    "ch3|sock|postwrite %p %p %p", sreq, conn, conn->vc); */
				goto fn_fail;
			    }
				    /* --END ERROR HANDLING-- */
			    
			    break;
			}
		}
	    } 
                        
	    break;
	}
	    	    
	case MPIDU_SCTP_OP_CLOSE:
	{

            MPIDI_VC_t* vc = (MPIDI_VC_t*) event-> user_ptr;  /* points to VC for close */

            mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
            if(mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            /* free vc->ch.pkt */
            if(vc->ch.pkt)
                MPIU_Free(vc->ch.pkt);
            vc->ch.pkt = NULL;

            /* TODO  add asserts? */
            	    
	    break;
	}
	

/* 	case MPIDU_SOCK_OP_WAKEUP: */
/* 	{ */
/* 	    MPIDI_CH3_Progress_signal_completion(); */
/* 	    /\* MPIDI_CH3I_progress_completion_count++; *\/ */
/* 	    break; */
/* 	} */

    case MPIDU_SCTP_OP_ACCEPT:
    {
        MPIDI_VC_t *vc; 
        MPIDI_CH3_Pkt_t * pkt = event->user_ptr;
        char * data_ptr = event->user_ptr;
        
        char host_description[MAX_HOST_DESCRIPTION_LEN];
        int port;
        MPIDU_Sock_ifaddr_t ifaddr;
        int hasIfaddr = 0;
        
        MPIU_Assert(pkt->type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT);

        /* allocate tmp VC.  this VC is used to exchange PG info w/ dynamic procs */
        vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
        /* --BEGIN ERROR HANDLING-- */
        if (vc == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
                                             __LINE__, MPI_ERR_OTHER,
                                             "**nomem", NULL);
            goto fn_exit;
        }

        /* VC marked as temporary (that what a NULL PG means) */
        MPIDI_VC_Init(vc, NULL, 0);
        MPIDI_CH3_VC_Init(vc);  /* TODO check success */

        /* data after pkt contains bizcard */
        data_ptr += sizeof(MPIDI_CH3_Pkt_t);
            
        mpi_errno = my_MPIDU_Sock_get_conninfo_from_bc( data_ptr, host_description,
                                                        sizeof(host_description),
                                                        &port, &ifaddr, &hasIfaddr );
        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        /*  save the sockaddr_in of new VC */
        giveMeSockAddr(ifaddr.ifaddr, port, &vc->ch.to_address);
               
        
        if(pkt->sc_conn_accept.ack) {
            /* this is the connect side */
            
            /* the fd was opened already, so that we could pass the SCTP port # to
             *  the accept side in the initial bizcard.
             */
            vc->ch.fd = MPIDI_CH3I_dynamic_tmp_fd;

            /* set so connect_to_root returns the correct VC */
            MPIDI_CH3I_dynamic_tmp_vc = vc;            

        } else {
            /* this is the accept side */

            /* the accept side may not have called MPI_Accept at this point and may
             *  have just been in the progress engine and received the accept request.
             *  Currently, we go ahead and create the new socket and ack to the
             *  connector with this new sockets info.  However, MPIDI_CH3I_dynamic_tmp_fd
             *  should not be set until this is dequeued from the acceptQ... which occurs
             *  in MPIDI_CH3_Complete_unidirectional_connection2
             */
            
            int tmp_fd, no_nagle, suggested_port, real_port;
	    int port_name_tag;
            struct sctp_event_subscribe evnts;
            MPIU_Size_t nb;

            union MPIDI_CH3_Pkt conn_acc_pkt;
            int iov_cnt = 2;
            MPID_IOV conn_acc_iov[iov_cnt];
            char bizcard[MPI_MAX_PORT_NAME];            
            MPID_IOV* iovp = conn_acc_iov;
            
            /* open new socket */            
            no_nagle = 1;
            suggested_port = 0;
            bzero(&evnts, sizeof(evnts));
            evnts.sctp_data_io_event=1;
            if(sctp_open_dgm_socket2(MPICH_SCTP_NUM_STREAMS,
                                     0, 5, suggested_port, no_nagle,
                                     &MPIDU_Sctpi_socket_bufsz, &evnts, &tmp_fd,
                                     &real_port) == -1) {
                /* FIXME define error code */
                goto fn_fail;
            }
            vc->ch.fd = tmp_fd;

            /*  the global tmp_fd shouldn't be set until dequeued from the accept
             *   queue since we only allow one to occur at a time (and like
             *   multiple_ports, they may happen out of order). now done in
             *   MPIDI_CH3_Complete_unidirectional_connection2
             */
/*             MPIDI_CH3I_dynamic_tmp_fd = tmp_fd; */


            /* store port temporarily so bizcard func works. put new tmp port in to
             *  pass to the connect side.
             */
            suggested_port = MPIDI_CH3I_listener_port;
            MPIDI_CH3I_listener_port = real_port;
            mpi_errno = MPIDI_CH3I_Get_business_card(bizcard, MPI_MAX_PORT_NAME);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                /* FIXME define error code */
                goto fn_fail;
            }
            /* --END ERROR HANDLING-- */
            MPIDI_CH3I_listener_port = suggested_port; /* restore */

    
            /* get the conn_acc_pkt ready */
            MPIDI_Pkt_init(&conn_acc_pkt, MPIDI_CH3I_PKT_SC_CONN_ACCEPT); 
            conn_acc_pkt.sc_conn_accept.bizcard_len = (int) strlen(bizcard) + 1; 
            conn_acc_pkt.sc_conn_accept.port_name_tag = pkt->sc_conn_accept.port_name_tag;
            port_name_tag = pkt->sc_conn_accept.port_name_tag);
            conn_acc_pkt.sc_conn_accept.ack = 1; /* this IS an ACK */

            /* get the iov ready */
            conn_acc_iov[0].MPID_IOV_BUF = (void *) &conn_acc_pkt;
            conn_acc_iov[0].MPID_IOV_LEN = sizeof(conn_acc_pkt);
            conn_acc_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) bizcard;
            conn_acc_iov[1].MPID_IOV_LEN = conn_acc_pkt.sc_conn_accept.bizcard_len;

            /* send conn_acc_pkt (ack=1) from my new fd to other side's new fd */
            for(;;) {
        
                mpi_errno = MPIDU_Sctp_writev_fd(tmp_fd,
                                                 &vc->ch.to_address, iovp,
                                                 iov_cnt, MPICH_SCTP_CTL_STREAM, 0, &nb );
                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS) {
                    goto fn_fail;
                }
                /* --END ERROR HANDLING-- */
                
                /* deliberately avoid nb < 0 */
                if(nb > 0 && adjust_iov(&iovp, &iov_cnt, nb)) { /* static in ch3_progress.c */
                    /* done sending */
                    break;
                }
            }
            
                
            /* put into acceptq and signal completion so upcalls in ch3u_port.c work */
            MPIDI_CH3I_Acceptq_enqueue(vc,port_name_tag);
	    MPIDI_CH3_Progress_signal_completion();
        }      
                        
        break;        
    }
    
    default:
	
	/* event has NO info at all, this is not an error */
	//printf("default handle event \n");

	break;

    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SCTP_EVENT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
/* end MPIDI_CH3I_Progress_handle_sock_event() */


#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_delay
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_delay(unsigned int completion_count)
{
    int mpi_errno = MPI_SUCCESS;
    
#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
	while (completion_count == MPIDI_CH3I_progress_completion_count)
	{
	    MPID_Thread_cond_wait(&MPIDI_CH3I_progress_completion_cond, &MPIR_Process.global_mutex);
	}
    }
#   endif
    
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_delay() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_continue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_continue(unsigned int completion_count)
{
    int mpi_errno = MPI_SUCCESS;

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
	MPID_Thread_cond_broadcast(&MPIDI_CH3I_progress_completion_cond);
    }
#   endif
    
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_continue() */

#endif /* (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL) */


#define MPIDI_MAX_KVS_KEY_LEN      256

/* myct: post_connect only retrieves the IP/PORT info from KVS now.
 * it can also send the connection packet, but not required to do it now */ 
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    char key[MPIDI_MAX_KVS_KEY_LEN];
    char val[MPIDI_MAX_KVS_VALUE_LEN];
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port;

    MPIDU_Sock_ifaddr_t ifaddr;

    int hasIfaddr = 0;
    int rc;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    /* this should not be called more than once per VC */
    MPIU_Assert(vc->ch.pkt == NULL);
    
    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CONNECTING");
    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    /* vc->pg can be NULL for temp VCs (used in dynamic processes). if this is the case,
     *  then we know here that the to_address is legit because in order to establish
     *  the temp VC, a conn_acc_pkt had to have been sent during connect/accept.
     */
    if(vc->pg != NULL) {
        /* "standard" VC */

        rc = MPIU_Snprintf(key, MPIDI_MAX_KVS_KEY_LEN, "P%d-businesscard", vc->pg_rank);
        /* --BEGIN ERROR HANDLING-- */
        if (rc < 0 || rc > MPIDI_MAX_KVS_KEY_LEN)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, 
                                             FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        mpi_errno = MPIDI_PG_GetConnString( vc->pg, vc->pg_rank, val, sizeof(val));

        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = my_MPIDU_Sock_get_conninfo_from_bc( val, host_description,
						 sizeof(host_description),
						 &port, &ifaddr, &hasIfaddr );
        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        /* save the sockaddr_in */
        giveMeSockAddr(ifaddr.ifaddr, port, &vc->ch.to_address);
    }

    /* setup the connection packet, and initialize iov arrays */
    
    vc->ch.pkt = (void*) MPIU_Malloc(sizeof(MPIDI_CH3_Pkt_t));
    if (vc->ch.pkt == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    MPIDI_CH3_Pkt_t* temp = vc->ch.pkt;    
    MPIDI_Pkt_init(temp, MPIDI_CH3I_PKT_SC_OPEN_REQ); 
    temp-> sc_open_req.pg_id_len = (int) strlen(MPIDI_Process.my_pg->id) + 1; 
    temp-> sc_open_req.pg_rank = MPIR_Process.comm_world->rank;

    int loop = 0; /* stream 0 is valid! */
    MPID_IOV* iov_ptr = NULL;

    // myct: loop invariant may be wrong. stream starts from 1
    /* any reason we need this per stream? */

    /* myct: mar27, this loop may not be needed, because connection_iov 
     * is used for something more general now
     */
    for(loop; loop < MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT; loop++) {
	iov_ptr = VC_IOV(vc, loop);
	iov_ptr[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) temp;
	iov_ptr[0].MPID_IOV_LEN = (int) sizeof(*temp);

	iov_ptr[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_Process.my_pg->id;
	iov_ptr[1].MPID_IOV_LEN = temp-> sc_open_req.pg_id_len;		       	
    }

    /* myct: optimization. init control stream purposely */
    MPIDU_Sctp_stream_init(vc, NULL, MPICH_SCTP_CTL_STREAM);
    

  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
 fn_fail:

    /* --BEGIN ERROR HANDLING-- */
    
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
/* end MPIDI_CH3I_VC_post_connect() */

/* myct: doesn't need this. we don't have anything dynamically allocated */
/* brad: don't understand this comment... */

#undef FUNCNAME
#define FUNCNAME stream_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int stream_post_sendq_req(MPIDI_VC_t * vc, int stream)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request* nextReq = NULL;
    MPIDI_STATE_DECL(MPID_STATE_STREAM_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_STREAM_POST_SENDQ_REQ);

    nextReq = MPIDI_CH3I_SendQ_head_x(vc, stream);
			
    if(nextReq) {
	MPIDI_CH3I_SendQ_dequeue_x(vc, stream);
	MPIDU_Sctp_post_writev(vc, nextReq, 0, NULL, stream);
	
    } else {
	SEND_ACTIVE(vc, stream) = NULL;
    }
        
    MPIDI_FUNC_EXIT(MPID_STATE_STREAM_POST_SENDQ_REQ);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt_and_pgid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_send_pkt_and_pgid(MPIDI_VC_t * vc, int stream)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    
    /* conn->iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &conn->pkt; */
/*     conn->iov[0].MPID_IOV_LEN = (int) sizeof(conn->pkt); */

/*     conn->iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_Process.my_pg->id; */
/*     conn->iov[1].MPID_IOV_LEN = (int) strlen(MPIDI_Process.my_pg->id) + 1; */

/*     mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->iov, 2, NULL); */
/*     if (mpi_errno != MPI_SUCCESS) { */
/* 	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail"); */
/*     } */
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
}

/* duplicate existed in sctp_util.c so made it non-static */

/* #undef FUNCNAME */
/* #define FUNCNAME adjust_iov */
/* #undef FCNAME */
/* #define FCNAME MPIU_QUOTE(FUNCNAME) */
/* static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb) */
/* { */
/*     MPID_IOV * const iov = *iovp; */
/*     const int count = *countp; */
/*     int offset = 0; */
    
/*     while (offset < count) */
/*     { */
/* 	if (iov[offset].MPID_IOV_LEN <= nb) */
/* 	{ */
/* 	    nb -= iov[offset].MPID_IOV_LEN; */
/* 	    offset++; */
/* 	} */
/* 	else */
/* 	{ */
/* 	    iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) iov[offset].MPID_IOV_BUF + nb); */
/* 	    iov[offset].MPID_IOV_LEN -= nb; */
/* 	    break; */
/* 	} */
/*     } */

/*     *iovp += offset; */
/*     *countp -= offset; */

/*     return (*countp == 0); */
/* } */
/* /\* end adjust_iov() *\/ */

#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIDU_Sctp_init(void) {
    char * env;
    long flags;
    struct sctp_initmsg initm;
    struct sctp_event_subscribe evnts;
    struct sockaddr_in addr;
    int port = 0;
    int rc;
    int no_nagle;
    int len;
    int mpi_errno = MPI_SUCCESS;

    
    /* FIXME TODO : change error codes to have SCTP errors */
    
    if (MPIDI_CH3I_onetomany_fd == -1)
    {
        /* See if a socket buffer size is specified */
        
	env = getenv("MPICH_SCTP_BUFFER_SIZE");
	if (env)
	{
	    int tmp;
	    
	    /* FIXME: atoi doesn't detect errors (e.g., non-digits) */
	    tmp = atoi(env);
	    if (tmp > 0)
	    {
		MPIDU_Sctpi_socket_bufsz = tmp;
	    }
	}

        
        /* Create a socket */
        
        /* set up parameters for the SCTP socket */
	no_nagle = 1;
	port = 0;
	bzero(&evnts, sizeof(evnts));
	evnts.sctp_data_io_event=1;
	
	MPIDU_Sctpi_socket_bufsz = 233016;

	if(sctp_open_dgm_socket2(MPICH_SCTP_NUM_STREAMS,
			     0, 5, port, no_nagle,
			     &MPIDU_Sctpi_socket_bufsz, &evnts, &MPIDI_CH3I_onetomany_fd,
				&MPIDI_CH3I_listener_port) == -1) {
	    goto fn_fail;
	}
	
    }
    

  fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (MPIDI_CH3I_onetomany_fd != -1)
    { 
	close(MPIDI_CH3I_onetomany_fd);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */   
}


/* myct: scheduling smartness can be implement here */
static int schedule_func(){
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Sctp_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/* static */ int MPIDU_Sctp_wait(int fd, int timeout, MPIDU_Sctp_event_t * event)
{
    /* set event->num_bytes to correspond to sctp_recvmsg return val */
    int msg_flags, error, recv_amount, stream_loop, buf_sz;
    MPIU_Size_t sz;
    int mpi_errno = MPI_SUCCESS;
    char* buf_ptr = NULL;
    MPIDI_VC_t * vc;
    struct MPID_Request* req = NULL;
    struct MPID_Request* q_tail = NULL;
    int blocked = FALSE;

    buf_sz = MPIDU_Sctpi_socket_bufsz;

    /* can't block if we don't know where things are coming from... */
    if(MPIDI_CH3I_dynamic_tmp_fd != -1)
        timeout = 0;
    
    /* buffer stuff */
    BufferNode_t* bf_node = NULL;

    /* END buffer stuff */
  
    while(MPIDU_Sctp_event_dequeue(event) != MPI_SUCCESS) {

	/* adjust sock mode */
	if(timeout == -1 && Global_SendQ.count == 0) {
	    sctp_setblock(fd, TRUE);
	    blocked = TRUE;
	}

	/* READ LOOP begins */
	BufferList_init(&FirstBufferNode);

	while((buf_ptr = request_buffer(CHUNK, &bf_node))) {
	    error = sctp_recv(fd, buf_ptr, CHUNK, &sctp_sri,
				    &msg_flags, &recv_amount);

	    if(error == EAGAIN || recv_amount <= 0) {
		break;
	    }

            
	    /* TODO check success */
	    MPIDU_Sctp_event_enqueue(MPIDU_SCTP_OP_READ,
				     recv_amount, &sctp_sri, fd, buf_ptr,
				     NULL, msg_flags, MPI_SUCCESS);
                
	    update_size(bf_node, recv_amount);
	    
	    error = 0;
	    recv_amount = 0;
	    
	    if(blocked) {
		sctp_setblock(fd, FALSE);
		blocked = FALSE;
		break;
	    }
	    
	}

	/* READ LOOP ends */
        
	sctp_setblock(fd, FALSE);

        /* read from dynamic_fd if it is exists and hasn't been tried already */
        if(MPIDI_CH3I_dynamic_tmp_fd != -1 && fd != MPIDI_CH3I_dynamic_tmp_fd) {
            fd = MPIDI_CH3I_dynamic_tmp_fd;
            continue;
        }

	/* WRITE LOOP begins */

	q_tail = Global_SendQ.tail;

	do {
	    req = NULL;
	    Global_SendQ_dequeue(req);

	    if(req) {

		MPIU_Assert(SEND_ACTIVE(req->ch.vc, req->ch.stream) == req);
	   
		/* keep sending until EAGAIN */
		stream_loop = req->ch.stream;
		SCTP_IOV* iov_ptr;
		vc = req->ch.vc;
		iov_ptr = &(vc->ch.posted_iov[stream_loop]);

		if(POST_IOV_FLAG(iov_ptr)) {
		    mpi_errno = MPIDU_Sctp_writev(vc, POST_IOV(iov_ptr),
						  POST_IOV_CNT(iov_ptr), req->ch.stream, 0, &sz);
		} else {
		    /* myct: NOT iov. do a simple write */
		    mpi_errno = MPIDU_Sctp_write(vc, POST_BUF(iov_ptr),
						 POST_BUF_MIN(iov_ptr), req->ch.stream, 0, &sz);
		    
		}
		
		sz = (sz < 0)? 0 : sz;

		/* adjust iov here, if it's done, enqueue event, else keep it
                 *  in global sendQ
                 */
		if(adjust_posted_iov(iov_ptr, sz)) {
		    mpi_errno = MPIDU_Sctp_event_enqueue(MPIDU_SCTP_OP_WRITE,
							 sz, NULL, vc->ch.fd, vc, NULL,
							 req->ch.stream, MPI_SUCCESS);

		    MPIDI_DBG_PRINTF((50, FCNAME, "wrote: %d bytes @ strm: %d", sz, req->ch.stream));
		}
		else {
		    /* myct: need to put it back to globalSendQ, doesn't need to post again*/
		    Global_SendQ_enqueue(vc, req, stream_loop);
		}
	    }
	    
	} while (req != q_tail);

	/* WRITE LOOP ends */

	/* myct: can't spin forever */
	if(!SPIN(timeout))
	    break;

    } 


    /* myct: set fd to NON_BLOCK again */
    sctp_setblock(fd, 0);

    return MPI_SUCCESS;

}


#undef FUNCNAME
#define FUNCNAME adjust_posted_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int adjust_posted_iov(SCTP_IOV* post_ptr, MPIU_Size_t nb) {

    int complete = 0;
    MPID_IOV* iovp = NULL;
    int iov_cnt;
    int min_bytes = 0;

    if(POST_IOV_FLAG(post_ptr)){
	iovp = POST_IOV(post_ptr);
	iov_cnt = POST_IOV_CNT(post_ptr);
	complete = adjust_iov(&iovp, &iov_cnt,
			      nb);
    } else {
	min_bytes = POST_BUF_MIN(post_ptr);

	if(min_bytes == nb) {
	    /* send complete */
	    complete = 1;
	} else {
	    POST_BUF(post_ptr) += nb;
	    POST_BUF_MIN(post_ptr) -= nb;
	    POST_BUF_MAX(post_ptr) = POST_BUF_MIN(post_ptr);
	    complete = 0;
	}	
    }

    return complete;
}


static int MPIDU_Sctp_post_close(MPIDI_VC_t * vc)
{
    /* sock enqueues an MPIDU_SOCK_OP_CLOSE event... */
    return MPIDU_Sctp_event_enqueue(MPIDU_SCTP_OP_CLOSE, 0, NULL, vc->ch.fd, vc, 0, 0, 0);
}

static int MPIDU_Sctp_finalize(void)
{
    /* need to free eventq  */
    if(eventq_head)
    {    
	MPIDU_Sctp_free_eventq_mem();
    }
    
    return MPI_SUCCESS;
}

inline static int read_from_advbuf_and_adjust(MPIDI_VC_t* vc, int stream, int amount,
				       char* src, MPID_Request* rreq) {

    int mpi_errno = MPI_SUCCESS;
    MPID_IOV* iovp = rreq->dev.iov;
    int nb = 0;
    int done = FALSE;
    int complete = FALSE;
    char *src_ptr;

    if(rreq) {
	nb = readv_from_advbuf(iovp, rreq->dev.iov_count, 
			       src, amount);
	
	done = adjust_iov(&iovp, &rreq->dev.iov_count, nb);
	
	if(done) {
            
#if 1
            int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
            reqFn = rreq->dev.OnDataAvail;
            if (!reqFn) {
                MPIDI_CH3U_Request_complete(rreq);
                complete = TRUE;
            }
            else {
                /* fyi reqFn is MPIDI_CH3_ReqHandler_ReloadIOV with truncated messages */
                mpi_errno = reqFn( vc, rreq, &complete );
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
#else
	    mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, rreq, &complete);
	    if (mpi_errno != MPI_SUCCESS) {
		MPIU_ERR_POP(mpi_errno);
	    }
#endif

	    if(!complete)
            {
                /* more data to be read (e.g. truncation) */
                
                /* this is designed to work with truncation, but is it general enough? */

                /* since the excess data is already in the advbuf, is this step necessary
                 *  for SCTP?  these steps (above and below) allocate a tmp buffer and set
                 *  it equal to the req's iov; this tmp buffer is used merely to get the
                 *  excess data off of the internal buffers (or the kernel socket receive
                 *  buffer, in SCTP's case).  the thing is, this is already done when
                 *  reading TO the advbuf, so this process of tmp buf allocation and copying
                 *  might be rework...  still, for now it's in there just to model after the
                 *  ch3:sock code.
                 */
                src_ptr = src;
                src_ptr += nb;
                nb = readv_from_advbuf(iovp, rreq->dev.iov_count, 
			       src_ptr, amount - nb);
                done = adjust_iov(&iovp, &rreq->dev.iov_count, nb);
                
                if(done) {
            
#if 1
                    reqFn = rreq->dev.OnDataAvail;
                    if (!reqFn) {
                        MPIDI_CH3U_Request_complete(rreq);
                        complete = TRUE;
                    }
                    else {
                        mpi_errno = reqFn( vc, rreq, &complete );
                        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                    }
#else
                    mpi_errno = MPIDI_CH3U_Handle_recv_req(vc, rreq, &complete);
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIU_ERR_POP(mpi_errno);
                    }
#endif
                }
            }
            if(complete)
            {
                RECV_ACTIVE(vc, stream) = NULL;
            }

	}
        if(!complete) {
	    RECV_ACTIVE(vc, stream) = rreq;
	}
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int MPIDI_CH3_Pending(MPIDI_VC_t* vc) {

    int i = 0;
    for(i; i < MPICH_SCTP_NUM_REQS_ACTIVE_TO_INIT; i++) {
	if(vc->ch.stream_table[i].sendQ_head)
	    return TRUE;
    }

    return FALSE;
}


int MPIDI_CH3_Channel_close( void )
{
    /* When called, Outstanding_close_ops in ch3u_handle_connection should be zero */
    /* WARNING! : Outstanding_close_ops can be zero prematurely if MPI_Comm_disconnect
     *              is called.
     */
    int mpi_errno = MPI_SUCCESS;

    /* sept29: is this code dated now that close is moved to MPIDI_CH3I_Progress_finalize ? */
       
    /* still have items in the sendQ so handle them before close */
    while(sendq_total)  /* FIXME might need to be more sophisticated with multiple fd's */
                        /*    For example, if the sendq_total is non-zero, we could have
                         *    writes outstanding on multiple fd's (the "normal" one and
                         *    the tmp one used for dynamic procs)
                         */
    {
/*         printf("sendQ not empty\n"); */
        int mpi_errno = MPI_SUCCESS;
        MPIDU_Sctp_event_t event2;
        mpi_errno = MPIDU_Sctp_wait(MPIDI_CH3I_onetomany_fd, MPIDU_SCTP_INFINITE_TIME,
                                    &event2);
        if (mpi_errno != MPI_SUCCESS)
        {
            MPIU_Assert(MPIR_ERR_GET_CLASS(mpi_errno) != MPIDU_SOCK_ERR_TIMEOUT);
            MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**progress_sock_wait");
            goto fn_fail;
        }
        mpi_errno = MPIDI_CH3I_Progress_handle_sctp_event(&event2);
        if (mpi_errno != MPI_SUCCESS) {
            MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
                                            "**ch3|sock|handle_sock_event");
        }
    }
                   
    if(MPIDI_CH3I_dynamic_tmp_vc) {
        /*  be sure to not close prematurely when a close pkt is received from
         *  a recently disconnected VC on the main onetomany socket
         */
        if(MPIDI_CH3I_dynamic_tmp_vc->state == MPIDI_VC_STATE_INACTIVE) {
            
        /* when using a tmp VC for dynamic processes, close the socket
         *  and reset variables since we only do one connect/accept pair at a time.
         */
            close(MPIDI_CH3I_dynamic_tmp_fd); /* FIXME check for errors */
            MPIDI_CH3I_dynamic_tmp_vc = NULL;
            MPIDI_CH3I_dynamic_tmp_fd = -1;
        }
    }
    /* close single one-to-many socket when all close ops are receieved on
     *  "normal" VC usage.  only close this if all VCs in all PGs are INACTIVE
     */
    else
    {
        /* moved to MPIDI_CH3I_Progress_finalize */
        
/*         MPIDI_PG_t * pg; */
/*         MPIDI_VC_t * vc; */
/*         int i, pg_size; */

/*         MPIDI_PG_Iterate_reset(); */
/*         MPIDI_PG_Get_next(&pg); */
/*         while(pg) { */
/*             i=0; */
/*             pg_size = pg->size; */
/*             while(i < pg_size) { */
/*                 if(pg->vct[i].state != MPIDI_VC_STATE_INACTIVE) */
/*                 { */
/*                     goto fn_exit;  /\* exit and don't close *\/ */
/*                 } */
/*                 i++; */
/*             } */
/*             MPIDI_PG_Get_next(&pg); */
/*         } */

/*         /\* FIXME - this will close the socket if all VCs are inactive */
/*          *            and we're not in finalize.  This should only happen */
/*          *            in finalize so this should be moved to */
/*          *            MPIDI_CH3I_Progress_finalize */
/*          *\/ */
/*         if (close(MPIDI_CH3I_onetomany_fd) == -1) {     */
/*             mpi_errno--;/\*FIXME need new error code *\/ */
/*             goto fn_fail; */
/*         } */
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


