/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "mpidu_sock.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#undef USE_CH3I_PROGRESS_DELAY_QUEUE


volatile unsigned int MPIDI_CH3I_progress_completion_count = 0;
#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    volatile int MPIDI_CH3I_progress_blocked = FALSE;
    volatile int MPIDI_CH3I_progress_wakeup_signalled = FALSE;

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
#       if defined(USE_CH3I_PROGRESS_DELAY_QUEUE)
            struct MPIDI_CH3I_Progress_delay_queue_elem
	    {
		unsigned int count;
		volatile int flag;
		MPID_Thread_cond_t cond;
		struct MPIDI_CH3I_Progress_delay_queue_elem * next;
	    };

            static struct MPIDI_CH3I_Progress_delay_queue_elem * MPIDI_CH3I_Progress_delay_queue_head = NULL;
            static struct MPIDI_CH3I_Progress_delay_queue_elem * MPIDI_CH3I_Progress_delay_queue_tail = NULL;
#       else
            MPID_Thread_cond_t MPIDI_CH3I_progress_completion_cond;
#       endif
#   endif
#endif


#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
    static int MPIDI_CH3I_Progress_delay(unsigned int completion_count);
    static int MPIDI_CH3I_Progress_continue(unsigned int completion_count);
#endif


MPIDU_Sock_set_t MPIDI_CH3I_sock_set = NULL; 
/* FIXME: We'll move the listener info out of this file soon */
#if 0
MPIDI_CH3I_Connection_t * MPIDI_CH3I_listener_conn = NULL;
#endif 
static int MPIDI_CH3I_Progress_handle_sock_event(MPIDU_Sock_event_t * event);

/* FIXME: move this prototype */
int MPIDI_CH3I_Connection_alloc(MPIDI_CH3I_Connection_t **);

static inline int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);

static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb);


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_test(void)
{
    MPIDU_Sock_event_t event;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);

#   if (MPICH_THREAD_LEVEL >= MPI_THREAD_MULTIPLE)
    {
	if (MPIDI_CH3I_progress_blocked == TRUE) 
	{
	    /*
	     * Another thread is already blocking in the progress engine.  
	     * We are not going to block waiting for progress, so we
	     * simply return.  It might make sense to yield before * returning,
	     * giving the PE thread a change to make progress.
	     *
	     * MT: Another thread is already blocking in poll.  Right now, 
	     * calls to the progress routines are effectively
	     * serialized by the device.  The only way another thread may 
	     * enter this function is if MPIDU_Sock_wait() blocks.  If
	     * this changes, a flag other than MPIDI_CH3I_Progress_blocked 
	     * may be required to determine if another thread is in
	     * the progress engine.
	     */
	    
	    goto fn_exit;
	}
    }
#   endif
    
    mpi_errno = MPIDU_Sock_wait(MPIDI_CH3I_sock_set, 0, &event);

    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_Progress_handle_sock_event(&event);
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
    MPIDU_Sock_event_t event;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PROGRESS_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PROGRESS_WAIT);

    /*
     * MT: the following code will be needed if progress can occur between 
     * MPIDI_CH3_Progress_start() and
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
	     * MT: Another thread is already blocking in poll.  Right now, 
	     * calls to MPIDI_CH3_Progress_wait() are effectively
	     * serialized by the device.  The only way another thread may 
	     * enter this function is if MPIDU_Sock_wait() blocks.  If
	     * this changes, a flag other than MPIDI_CH3I_Progress_blocked 
	     * may be required to determine if another thread is in
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
	
	mpi_errno = MPIDU_Sock_wait(MPIDI_CH3I_sock_set, MPIDU_SOCK_INFINITE_TIME, &event);

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

	mpi_errno = MPIDI_CH3I_Progress_handle_sock_event(&event);
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
	 * Awaken any threads which are waiting for the progress that just 
	 * occurred
	 */
	MPIDI_CH3I_Progress_continue(MPIDI_CH3I_progress_completion_count);
    }
#   endif
    
 fn_exit:
    /*
     * Reset the progress state so it is fresh for the next iteration
     */
    progress_state->ch.completion_count = MPIDI_CH3I_progress_completion_count;
    
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
    vc->ch.conn->state = CONN_STATE_CLOSING;
    mpi_errno = MPIDU_Sock_post_close(vc->ch.sock);
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
int MPIDI_CH3I_Progress_init(void)
{
    MPIDU_Sock_t sock;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX && !defined(USE_CH3I_PROGRESS_DELAY_QUEUE))
    {
	MPID_Thread_cond_create(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif
	
    mpi_errno = MPIDU_Sock_init();
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    
    /* create sock set */
    mpi_errno = MPIDU_Sock_create_set(&MPIDI_CH3I_sock_set);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    
    /* establish non-blocking listener */
    /* FIXME: Move into socket connection code */
#if 0
    mpi_errno = MPIDI_CH3I_Connection_alloc(&MPIDI_CH3I_listener_conn);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting listener connect state to CONN_STATE_LISTENING");
    MPIDI_CH3I_listener_conn->sock = NULL;
    MPIDI_CH3I_listener_conn->vc = NULL;
    MPIDI_CH3I_listener_conn->state = CONN_STATE_LISTENING;
    MPIDI_CH3I_listener_conn->send_active = NULL;
    MPIDI_CH3I_listener_conn->recv_active = NULL;
    
    mpi_errno = MPIDU_Sock_listen(MPIDI_CH3I_sock_set, MPIDI_CH3I_listener_conn, &MPIDI_CH3I_listener_port, &sock);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    
    MPIDI_CH3I_listener_conn->sock = sock;
#else
    mpi_errno = MPIDU_CH3I_SetupListener( MPIDI_CH3I_sock_set );
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
#endif
    
  fn_exit:
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
    int mpi_errno;
    MPID_Progress_state progress_state;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    /* Shut down the listener */
#if 0
    mpi_errno = MPIDU_Sock_post_close(MPIDI_CH3I_listener_conn->sock);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    
    MPID_Progress_start(&progress_state);
    while(MPIDI_CH3I_listener_conn != NULL)
    {
	mpi_errno = MPID_Progress_wait(&progress_state);
	
    }
    MPID_Progress_end(&progress_state);
#else
    MPIDU_CH3I_ShutdownListener();
#endif
    
    /* FIXME: Cleanly shutdown other socks and free connection structures. 
       (close protocol?) */


    /*
     * MT: in a multi-threaded environment, finalize() should signal any 
     * thread(s) blocking on MPIDU_Sock_wait() and wait for
     * those * threads to complete before destroying the progress engine 
     * data structures.
     */

    MPIDU_Sock_destroy_set(MPIDI_CH3I_sock_set);
    MPIDU_Sock_finalize();

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX && !defined(USE_CH3I_PROGRESS_DELAY_QUEUE))
    {
	MPID_Thread_cond_destroy(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif
    
  fn_exit:
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
    return MPIDI_CH3U_Get_business_card_sock(&value, &length);
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_handle_sock_event
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_CH3I_Progress_handle_sock_event(MPIDU_Sock_event_t * event)
{
    int complete;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);

    MPIU_DBG_MSG_D(CH3,VERBOSE,"Socket event of type %d", event->op_type );

    switch (event->op_type)
    {
	case MPIDU_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
		
	    MPID_Request * rreq = conn->recv_active;

	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS)
	    {
		/* FIXME: the following should be handled by the close 
		   protocol */
		if (MPIR_ERR_GET_CLASS(event->error) != MPIDU_SOCK_ERR_CONN_CLOSED) {
		    mpi_errno = event->error;
		    MPIU_ERR_POP(mpi_errno);
		}		    
		break;
	    }
	    /* --END ERROR HANDLING-- */
		
	    if (conn->state == CONN_STATE_CONNECTED)
	    {
		if (conn->recv_active == NULL)
		{
		    MPIU_Assert(conn->pkt.type < MPIDI_CH3_PKT_END_CH3);
			
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt, &rreq);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }

		    if (rreq == NULL)
		    {
			if (conn->state != CONN_STATE_CLOSING)
			{
			    /* conn->recv_active = NULL;  -- already set to NULL */
			    mpi_errno = connection_post_recv_pkt(conn);
			    if (mpi_errno != MPI_SUCCESS) {
				MPIU_ERR_POP(mpi_errno);
			    }
			}
		    }
		    else
		    {
			for(;;)
			{
			    MPID_IOV * iovp;
			    MPIU_Size_t nb;
				
			    iovp = rreq->dev.iov;
			    
			    mpi_errno = MPIDU_Sock_readv(conn->sock, iovp, rreq->dev.iov_count, &nb);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_fail;
			    }
			    /* --END ERROR HANDLING-- */

			    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
    (MPIU_DBG_FDEST,"immediate readv, vc=%p nb=%d, rreq=0x%08x",
     conn->vc, nb, rreq->handle));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
				if (mpi_errno != MPI_SUCCESS) {
				    MPIU_ERR_POP(mpi_errno);
				}

				if (complete)
				{
				    /* conn->recv_active = NULL; -- already set to NULL */
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS) {
					MPIU_ERR_POP(mpi_errno);
				    }

				    break;
				}
			    }
			    else
			    {
				MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
        (MPIU_DBG_FDEST,"posting readv, vc=%p, rreq=0x%08x", 
	 conn->vc, rreq->handle));
				conn->recv_active = rreq;
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_fail;
				}
				/* --END ERROR HANDLING-- */

				break;
			    }
			}
		    }
		}
		else /* incoming data */
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
			
		    if (complete)
		    {
			conn->recv_active = NULL;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS) {
			    MPIU_ERR_POP(mpi_errno);
			}
		    }
		    else /* more data to be read */
		    {
			for(;;)
			{
			    MPID_IOV * iovp;
			    MPIU_Size_t nb;
				
			    iovp = rreq->dev.iov;
			    
			    mpi_errno = MPIDU_Sock_readv(conn->sock, iovp, rreq->dev.iov_count, &nb);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_fail;
			    }
			    /* --END ERROR HANDLING-- */

			    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
        (MPIU_DBG_FDEST,"immediate readv, vc=%p nb=%d, rreq=0x%08x",
	 conn->vc, rreq->handle, nb));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
				if (mpi_errno != MPI_SUCCESS) {
				    MPIU_ERR_POP(mpi_errno);
				}

				if (complete)
				{
				    conn->recv_active = NULL;
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS) {
					MPIU_ERR_POP(mpi_errno);
				    }

				    break;
				}
			    }
			    else
			    {
				MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
     (MPIU_DBG_FDEST,"posting readv, vc=%p, rreq=0x%08x", 
      conn->vc, rreq->handle));
				/* conn->recv_active = rreq;  -- already set to current request */
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_fail;
				}
				/* --END ERROR HANDLING-- */

				break;
			    }
			}
		    }
		}
	    }
	    else if (conn->state == CONN_STATE_OPEN_LRECV_DATA)
	    {
		mpi_errno = MPIDI_CH3_Sockconn_handle_connopen_event( conn );
		if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }
	    }
	    else /* Handling some internal connection establishment or tear down packet */
	    { 
		mpi_errno = MPIDI_CH3_Sockconn_handle_conn_event( conn );
		if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    }
	    break;
	}

	/* END OF SOCK_OP_READ */

	case MPIDU_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
	    /* --BEGIN ERROR HANDLING-- */
	    if (event->error != MPI_SUCCESS) {
		mpi_errno = event->error;
		MPIU_ERR_POP(mpi_errno);
	    }
	    /* --END ERROR HANDLING-- */
		
	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
		if (mpi_errno != MPI_SUCCESS) {
		    MPIU_ERR_POP(mpi_errno);
		}
		    
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(conn->vc);
		    mpi_errno = connection_post_sendq_req(conn);
		    if (mpi_errno != MPI_SUCCESS) {
			MPIU_ERR_POP(mpi_errno);
		    }
		}
		else /* more data to send */
		{
		    for(;;)
		    {
			MPID_IOV * iovp;
			MPIU_Size_t nb;
				
			iovp = sreq->dev.iov;
			    
			mpi_errno = MPIDU_Sock_writev(conn->sock, iovp, sreq->dev.iov_count, &nb);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|immedwrite", "ch3|sock|immedwrite %p %p %p",
							     sreq, conn, conn->vc);
			    goto fn_fail;
			}
			/* --END ERROR HANDLING-- */

			MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
       (MPIU_DBG_FDEST,"immediate writev, vc=%p, sreq=0x%08x, nb=%d",
	conn->vc, sreq->handle, nb));
			    
			if (nb > 0 && adjust_iov(&iovp, &sreq->dev.iov_count, nb))
			{
			    mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
			    if (mpi_errno != MPI_SUCCESS) {
				MPIU_ERR_POP(mpi_errno);
			    }

			    if (complete)
			    {
				MPIDI_CH3I_SendQ_dequeue(conn->vc);
				mpi_errno = connection_post_sendq_req(conn);
				if (mpi_errno != MPI_SUCCESS) {
				    MPIU_ERR_POP(mpi_errno);
				}
				break;
			    }
			}
			else
			{
			    MPIU_DBG_MSG_FMT(CH3_CHANNEL,VERBOSE,
       (MPIU_DBG_FDEST,"posting writev, vc=%p, sreq=0x%08x", 
	conn->vc, sreq->handle));
			    mpi_errno = MPIDU_Sock_post_writev(conn->sock, iovp, sreq->dev.iov_count, NULL);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postwrite",
				    "ch3|sock|postwrite %p %p %p", sreq, conn, conn->vc);
				goto fn_fail;
			    }
			    /* --END ERROR HANDLING-- */

			    break;
			}
		    }
		}
	    }
	    else /* finished writing internal packet header */
	    {
		mpi_errno = MPIDI_CH3_Sockconn_handle_connwrite( conn );
		if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }
	    }
	    break;
	}
	/* END OF SOCK_OP_WRITE */

	case MPIDU_SOCK_OP_ACCEPT:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_accept_event();
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	}
	    
	case MPIDU_SOCK_OP_CONNECT:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_connect_event( 
				(MPIDI_CH3I_Connection_t *) event->user_ptr,
				event->error );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	}
	    
	case MPIDU_SOCK_OP_CLOSE:
	{
	    mpi_errno = MPIDI_CH3_Sockconn_handle_close_event( 
			      (MPIDI_CH3I_Connection_t *) event->user_ptr );
	    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
	    break;
	}

	case MPIDU_SOCK_OP_WAKEUP:
	{
	    MPIDI_CH3_Progress_signal_completion();
	    /* MPIDI_CH3I_progress_completion_count++; */
	    break;
	}
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);
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
    
#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MONITOR)
    {
#	error This is so not right.  But what is the correct technique?
	
	if (MPIU_Monitor_closet_get_occupancy_count(MPIR_Process.global_closet) > 0)
	{
	    MPIU_Monitor_continue(MPIR_Process.global_monitor, MPIR_Process.global_closet);
	    MPIU_Monitor_enter(MPIR_Process.global_monitor);
	    if (completion_count != MPIDI_CH3I_progress_completion_count)
	    {
		goto impl_exit;
	    }
	}
		    
	MPIU_Monitor_delay(MPIR_Process.global_monitor, MPIR_Process.global_closet);

      impl_exit:
	{
	}
    }
#   elif (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
#	if defined(USE_CH3I_PROGRESS_DELAY_QUEUE)
	{
	    int rc;
	    struct MPIDI_CH3I_Progress_delay_queue_elem dq_elem;
	
	    dq_elem.count = completion_count;
	    dq_elem.flag = FALSE;
    
	    dq_elem.next = NULL;
	    MPIDI_CH3I_Progress_delay_queue_tail->next = &dq_elem;
	    MPIDI_CH3I_Progress_delay_queue_tail = &dq_elem;
	    if (MPIDI_CH3I_Progress_delay_queue_head == NULL)
	    {
		MPIDI_CH3I_Progress_delay_queue_head = &dq_elem;
	    }

	    rc = MPID_Thread_cond_create(&dq_elem.cond, NULL);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rc != 0)
	    { 
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
		goto impl_exit;
	    }
	    /* --END ERROR HANDLING-- */
    
	    do
	    {
		MPID_Thread_cond_wait(&dq_elem.cond, &MPIR_Process.global_mutex);
	    }
	    while(dq_elem.flag == FALSE);
	    
	    MPID_Thread_cond_destroy(&dq_elem.cond, NULL);
	    
	  impl_exit:
	    {
	    }
	}
#	else
	{ 
	    while (completion_count == MPIDI_CH3I_progress_completion_count)
	    {
		MPID_Thread_cond_wait(&MPIDI_CH3I_progress_completion_cond, &MPIR_Process.global_mutex);
	    }
	}
#       endif
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

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MONITOR)
    {
#	error This is so not right.  But what is the correct technique?
	if (MPIU_Monitor_closet_get_occupancy(MPIR_Process.global_closet) > 0)
	{
	    MPIU_Monitor_continue(MPIR_Process.global_monitor, MPIR_Process.global_closet);
	}
	else
	{ 
	    MPIU_Monitor_exit(MPIR_Process.global_monitor);
	}
    }
#   elif (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
    {
#	if defined(USE_CH3I_PROGRESS_DELAY_QUEUE)
	{
	    struct MPIDI_CH3I_Progress_delay_queue_elem * dq_elem;
	    
	    dq_elem = MPIDI_CH3I_Progress_delay_queue_head;
	    while(dq_elem != NULL && dq_elem->count != completion_count)
	    {
		dq_elem->flag = TRUE;
		MPID_Thread_cond_signal(&dq_elem->cond);
		dq_elem = dq_elem->next;
	    }
	    MPIDI_CH3I_Progress_delay_queue_head = dq_elem;
	}
#	else
	{
	    MPID_Thread_cond_broadcast(&MPIDI_CH3I_progress_completion_cond);
	}
#	endif
    }
#   endif
    
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_continue() */

#endif /* (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL) */


/* FIXME: (a) what does this do and where is it used and (b) 
   we could replace it with a #define for the single-method case */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    return MPIDI_CH3I_VC_post_sockconnect( vc );
}
/* end MPIDI_CH3I_VC_post_connect() */

#undef FUNCNAME
#define FUNCNAME connection_post_sendq_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SENDQ_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    /* post send of next request on the send queue */
    conn->send_active = MPIDI_CH3I_SendQ_head(conn->vc); /* MT */
    if (conn->send_active != NULL)
    {
	mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->send_active->dev.iov, conn->send_active->dev.iov_count, NULL);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	}
	/* --END ERROR HANDLING-- */
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    return mpi_errno;
}



#if 1
#undef FUNCNAME
#define FUNCNAME connection_post_recv_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_RECV_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_RECV_PKT);

    mpi_errno = MPIDU_Sock_post_read(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
    return mpi_errno;
}
#endif


/* FIXME: What is this routine for? */
#undef FUNCNAME
#define FUNCNAME adjust_iov
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb)
{
    MPID_IOV * const iov = *iovp;
    const int count = *countp;
    int offset = 0;
    
    while (offset < count)
    {
	if (iov[offset].MPID_IOV_LEN <= nb)
	{
	    nb -= iov[offset].MPID_IOV_LEN;
	    offset++;
	}
	else
	{
	    iov[offset].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)((char *) iov[offset].MPID_IOV_BUF + nb);
	    iov[offset].MPID_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}
/* end adjust_iov() */
