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
#define MAX_HOST_DESCRIPTION_LEN 256


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


static MPIDU_Sock_set_t sock_set; 
static int MPIDI_CH3I_listener_port = 0;
static MPIDI_CH3I_Connection_t * MPIDI_CH3I_listener_conn = NULL;

static int MPIDI_CH3I_Progress_handle_sock_event(MPIDU_Sock_event_t * event);

static inline int connection_alloc(MPIDI_CH3I_Connection_t **);
static inline void connection_free(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn);
static inline int connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);
static inline int connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
static int connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
static inline void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn);
static int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb);


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Progress_test
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Progress_test()
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
    
    mpi_errno = MPIDU_Sock_wait(sock_set, 0, &event);

    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPIDI_CH3I_Progress_handle_sock_event(&event);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|handle_sock_event", NULL);
	    goto fn_exit;
	}
    }
    else if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDU_SOCK_ERR_TIMEOUT)
    {
	mpi_errno = MPI_SUCCESS;
	goto fn_exit;
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**progress_sock_wait", NULL);
	goto fn_exit;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PROGRESS_TEST);
    return mpi_errno;
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
	
	mpi_errno = MPIDU_Sock_wait(sock_set, MPIDU_SOCK_INFINITE_TIME, &event);

#       if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
	{
	    MPIDI_CH3I_progress_blocked = FALSE;
	    MPIDI_CH3I_progress_wakeup_signalled = FALSE;
	}
#	endif
	
	if (mpi_errno != MPI_SUCCESS)
	{
	    MPIU_Assert(MPIR_ERR_GET_CLASS(mpi_errno) != MPIDU_SOCK_ERR_TIMEOUT);
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**progress_sock_wait", NULL);
	    goto fn_exit;
	}

	mpi_errno = MPIDI_CH3I_Progress_handle_sock_event(&event);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					     "**ch3|sock|handle_sock_event", NULL);
	    goto fn_exit;
	}
    }
    while (progress_state->ch.completion_count == MPIDI_CH3I_progress_completion_count);

    /*
     * We could continue to call MPIU_Sock_wait in a non-blocking fashion and process any other events; however, this would not
     * give the application a chance to post new receives, and thus could result in an increased number of unexpected messages
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
}
/* end MPIDI_CH3_Progress_wait() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connection_terminate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connection_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    
    vc->ch.conn->state = CONN_STATE_CLOSING;
    mpi_errno = MPIDU_Sock_post_close(vc->ch.sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }

  fn_exit:
    return mpi_errno;
}
/* end MPIDI_CH3_Connection_terminate() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_init()
{
    MPIDU_Sock_t sock;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX && !defined(USE_CH3I_PROGRESS_DELAY_QUEUE))
    {
	MPID_Thread_cond_create(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif
	
    mpi_errno = MPIDU_Sock_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    
    /* create sock set */
    mpi_errno = MPIDU_Sock_create_set(&sock_set);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    
    /* establish non-blocking listener */
    mpi_errno = connection_alloc(&MPIDI_CH3I_listener_conn);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    MPIDI_CH3I_listener_conn->sock = NULL;
    MPIDI_CH3I_listener_conn->vc = NULL;
    MPIDI_CH3I_listener_conn->state = CONN_STATE_LISTENING;
    MPIDI_CH3I_listener_conn->send_active = NULL;
    MPIDI_CH3I_listener_conn->recv_active = NULL;
    
    mpi_errno = MPIDU_Sock_listen(sock_set, MPIDI_CH3I_listener_conn, &MPIDI_CH3I_listener_port, &sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    
    MPIDI_CH3I_listener_conn->sock = sock;
    
  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_INIT);
    return mpi_errno;
}
/* end MIPDI_CH3I_Progress_init() */


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Progress_finalize()
{
    int mpi_errno;
    MPID_Progress_state progress_state;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    /* Shut down the listener */
    mpi_errno = MPIDU_Sock_post_close(MPIDI_CH3I_listener_conn->sock);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }
    
    MPID_Progress_start(&progress_state);
    while(MPIDI_CH3I_listener_conn != NULL)
    {
	mpi_errno = MPID_Progress_wait(&progress_state);
	
    }
    MPID_Progress_end(&progress_state);
    
    /* FIXME: Cleanly shutdown other socks and free connection structures. (close protocol?) */


    /*
     * MT: in a multi-threaded environment, finalize() should signal any thread(s) blocking on MPIDU_Sock_wait() and wait for
     * those * threads to complete before destroying the progress engine data structures.
     */

    MPIDU_Sock_destroy_set(sock_set);
    MPIDU_Sock_finalize();

#   if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX && !defined(USE_CH3I_PROGRESS_DELAY_QUEUE))
    {
	MPID_Thread_cond_destroy(&MPIDI_CH3I_progress_completion_cond, NULL);
    }
#   endif
    
  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_FINALIZE);
    return mpi_errno;
}
/* end MPIDI_CH3I_Progress_finalize() */



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Progress_wakeup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3I_Progress_wakeup(void)
{
    MPIDU_Sock_wakeup(sock_set);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Get_business_card(char *value, int length)
{
    int mpi_errno;
    char host_description[MAX_HOST_DESCRIPTION_LEN];

    mpi_errno = MPIDU_Sock_get_host_description(host_description, MAX_HOST_DESCRIPTION_LEN);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_description", 0);
	return mpi_errno;
    }
    
    mpi_errno = MPIU_Str_add_int_arg(&value, &length, MPIDI_CH3I_PORT_KEY, (int) MPIDI_CH3I_listener_port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    
    mpi_errno = MPIU_Str_add_string_arg(&value, &length, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard_len", 0);
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**buscard", 0);
	}
	return mpi_errno;
    }
    return MPI_SUCCESS;
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

    switch (event->op_type)
    {
	case MPIDU_SOCK_OP_READ:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
		
	    MPID_Request * rreq = conn->recv_active;
		
	    if (event->error != MPI_SUCCESS)
	    {
		/* FIXME: the following should be handled by the close protocol */
		if (MPIR_ERR_GET_CLASS(event->error) != MPIDU_SOCK_ERR_CONN_CLOSED)
		{
		    mpi_errno = connection_recv_fail(conn, event->error);
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", NULL);
		    goto fn_exit;
		}
		    
		break;
	    }
		
	    if (conn->state == CONN_STATE_CONNECTED)
	    {
		if (conn->recv_active == NULL)
		{
		    MPIU_Assert(conn->pkt.type < MPIDI_CH3_PKT_END_CH3);
			
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(conn->vc, &conn->pkt, &rreq);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**fail", NULL);
			goto fn_exit;
		    }

		    if (rreq == NULL)
		    {
			if (conn->state != CONN_STATE_CLOSING)
			{
			    /* conn->recv_active = NULL;  -- already set to NULL */
			    mpi_errno = connection_post_recv_pkt(conn);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
				    "**fail", NULL);
				goto fn_exit;
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
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_exit;
			    }

			    MPIDI_DBG_PRINTF((55, FCNAME, "immediate readv, vc=0x%p nb=%d, rreq=0x%08x",
					      conn->vc, rreq->handle, nb));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
				    goto fn_exit;
				}

				if (complete)
				{
				    /* conn->recv_active = NULL; -- already set to NULL */
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS)
				    {
					mpi_errno = MPIR_Err_create_code(
					    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**fail", NULL);
					goto fn_exit;
				    }

				    break;
				}
			    }
			    else
			    {
				MPIDI_DBG_PRINTF((55, FCNAME, "posting readv, vc=0x%p, rreq=0x%08x", conn->vc, rreq->handle));
				conn->recv_active = rreq;
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_exit;
				}

				break;
			    }
			}
		    }
		}
		else /* incoming data */
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**fail", NULL);
			goto fn_exit;
		    }
			
		    if (complete)
		    {
			conn->recv_active = NULL;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**fail", NULL);
			    goto fn_exit;
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
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
								 "**ch3|sock|immedread", "ch3|sock|immedread %p %p %p",
								 rreq, conn, conn->vc);
				goto fn_exit;
			    }

			    MPIDI_DBG_PRINTF((55, FCNAME, "immediate readv, vc=0x%p nb=%d, rreq=0x%08x",
					      conn->vc, rreq->handle, nb));
				
			    if (nb > 0 && adjust_iov(&iovp, &rreq->dev.iov_count, nb))
			    {
				mpi_errno = MPIDI_CH3U_Handle_recv_req(conn->vc, rreq, &complete);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
				    goto fn_exit;
				}

				if (complete)
				{
				    conn->recv_active = NULL;
				    mpi_errno = connection_post_recv_pkt(conn);
				    if (mpi_errno != MPI_SUCCESS)
				    {
					mpi_errno = MPIR_Err_create_code(
					    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**fail", NULL);
					goto fn_exit;
				    }

				    break;
				}
			    }
			    else
			    {
				MPIDI_DBG_PRINTF((55, FCNAME, "posting readv, vc=0x%p, rreq=0x%08x", conn->vc, rreq->handle));
				/* conn->recv_active = rreq;  -- already set to current request */
				mpi_errno = MPIDU_Sock_post_readv(conn->sock, iovp, rreq->dev.iov_count, NULL);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postread",
					"ch3|sock|postread %p %p %p", rreq, conn, conn->vc);
				    goto fn_exit;
				}

				break;
			    }
			}
		    }
		}
	    }
	    else if (conn->state == CONN_STATE_OPEN_LRECV_DATA)
	    {
		MPIDI_PG_t * pg;
		int pg_rank;
		MPIDI_VC_t * vc;

		/* Look up pg based on conn->pg_id */
		mpi_errno = MPIDI_PG_Find(conn->pg_id, &pg);
		if (pg == NULL)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**pglookup", "**pglookup %s", conn->pg_id);
		    goto fn_exit;
		}

		pg_rank = conn->pkt.sc_open_req.pg_rank;
		MPIDI_PG_Get_vc(pg, pg_rank, &vc);
		MPIU_Assert(vc->pg_rank == pg_rank);
                    
		if (vc->ch.conn == NULL)
		{
		    /* no head-to-head connects, accept the
		       connection */
		    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		    vc->ch.sock = conn->sock;
		    vc->ch.conn = conn;
		    conn->vc = vc;
                        
		    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		    conn->pkt.sc_open_resp.ack = TRUE;
		}
		else
		{
		    /* head to head situation */
		    if (pg == MPIDI_Process.my_pg)
		    {
			/* the other process is in the same comm_world; just compare the ranks */
			if (MPIR_Process.comm_world->rank < pg_rank)
			{
			    /* accept connection */
			    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->ch.sock = conn->sock;
			    vc->ch.conn = conn;
			    conn->vc = vc;
                                
			    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
			    conn->pkt.sc_open_resp.ack = TRUE;
			}
			else
			{
			    /* refuse connection */
			    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
			    conn->pkt.sc_open_resp.ack = FALSE;
			}
		    }
		    else
		    { 
			/* the two processes are in different comm_worlds; compare their unique pg_ids. */
			if (strcmp(MPIDI_Process.my_pg->id, pg->id) < 0)
			{
			    /* accept connection */
			    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
			    vc->ch.sock = conn->sock;
			    vc->ch.conn = conn;
			    conn->vc = vc;
                                
			    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
			    conn->pkt.sc_open_resp.ack = TRUE;
			}
			else
			{
			    /* refuse connection */
			    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
			    conn->pkt.sc_open_resp.ack = FALSE;
			}
		    }
		}
                    
		conn->state = CONN_STATE_OPEN_LSEND;
		mpi_errno = connection_post_send_pkt(conn);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|open_lrecv_data", NULL);
		    goto fn_exit;
		}
	    }
	    else /* Handling some internal connection establishment or tear down packet */
	    { 
		if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_REQ)
		{
		    conn->state = CONN_STATE_OPEN_LRECV_DATA;
		    mpi_errno = MPIDU_Sock_post_read(conn->sock, conn->pg_id, conn->pkt.sc_open_req.pg_id_len, 
						     conn->pkt.sc_open_req.pg_id_len, NULL);   
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = connection_recv_fail(conn, mpi_errno);
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**fail", NULL);
			goto fn_exit;
		    }
		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_CONN_ACCEPT)
		{
		    MPIDI_VC_t *vc; 

		    vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
		    if (vc == NULL)
		    {
			mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**nomem", NULL);
			goto fn_exit;
		    }
		    /* FIXME - where does this vc get freed? */

		    MPIDI_VC_Init(vc, NULL, 0);
		    vc->ch.sendq_head = NULL;
		    vc->ch.sendq_tail = NULL;
		    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
		    vc->ch.sock = conn->sock;
		    vc->ch.conn = conn;
		    conn->vc = vc;
                        
		    MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_RESP);
		    conn->pkt.sc_open_resp.ack = TRUE;
                        
		    conn->state = CONN_STATE_OPEN_LSEND;
		    mpi_errno = connection_post_send_pkt(conn);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**ch3|sock|scconnaccept", NULL);
			goto fn_exit;
		    }

		    /* ENQUEUE vc */
		    MPIDI_CH3I_Acceptq_enqueue(vc);

		}
		else if (conn->pkt.type == MPIDI_CH3I_PKT_SC_OPEN_RESP)
		{
		    if (conn->pkt.sc_open_resp.ack)
		    {
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			MPIU_Assert(conn->vc->ch.conn == conn);
			MPIU_Assert(conn->vc->ch.sock == conn->sock);
			    
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**fail", NULL);
			    goto fn_exit;
			}
			mpi_errno = connection_post_sendq_req(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**ch3|sock|scopenresp", NULL);
			    goto fn_exit;
			}
		    }
		    else
		    {
			conn->vc = NULL;
			conn->state = CONN_STATE_CLOSING;
			MPIDU_Sock_post_close(conn->sock);
		    }
		}
		else
		{
		    MPIDI_DBG_Print_packet(&conn->pkt);
		    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|badpacket", "**ch3|sock|badpacket %d", conn->pkt.type);
		    goto fn_exit;
		}
	    }

	    break;
	}
	    
	case MPIDU_SOCK_OP_WRITE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;

	    if (event->error != MPI_SUCCESS)
	    {
		mpi_errno = connection_send_fail(conn, event->error);
		goto fn_exit;
	    }
		
	    if (conn->send_active)
	    {
		MPID_Request * sreq = conn->send_active;

		mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**fail", NULL);
		    goto fn_exit;
		}
		    
		if (complete)
		{
		    MPIDI_CH3I_SendQ_dequeue(conn->vc);
		    mpi_errno = connection_post_sendq_req(conn);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**fail", NULL);
			goto fn_exit;
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
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**ch3|sock|immedwrite", "ch3|sock|immedwrite %p %p %p",
							     sreq, conn, conn->vc);
			    goto fn_exit;
			}

			MPIDI_DBG_PRINTF((55, FCNAME, "immediate writev, vc=0x%p, sreq=0x%08x, nb=%d",
					  conn->vc, sreq->handle, nb));
			    
			if (nb > 0 && adjust_iov(&iovp, &sreq->dev.iov_count, nb))
			{
			    mpi_errno = MPIDI_CH3U_Handle_send_req(conn->vc, sreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
				goto fn_exit;
			    }

			    if (complete)
			    {
				MPIDI_CH3I_SendQ_dequeue(conn->vc);
				mpi_errno = connection_post_sendq_req(conn);
				if (mpi_errno != MPI_SUCCESS)
				{
				    mpi_errno = MPIR_Err_create_code(
					mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**fail", NULL);
				    goto fn_exit;
				}

				break;
			    }
			}
			else
			{
			    MPIDI_DBG_PRINTF((55, FCNAME, "posting writev, vc=0x%p, sreq=0x%08x", conn->vc, sreq->handle));
			    mpi_errno = MPIDU_Sock_post_writev(conn->sock, iovp, sreq->dev.iov_count, NULL);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(
				    mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postwrite",
				    "ch3|sock|postwrite %p %p %p", sreq, conn, conn->vc);
				goto fn_exit;
			    }

			    break;
			}
		    }
		}
	    }
	    else /* finished writing internal packet header */
	    {
		if (conn->state == CONN_STATE_OPEN_CSEND)
		{
		    /* finished sending open request packet */
		    /* post receive for open response packet */
		    conn->state = CONN_STATE_OPEN_CRECV;
		    mpi_errno = connection_post_recv_pkt(conn);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							 "**fail", NULL);
			goto fn_exit;
		    }
		}
		else if (conn->state == CONN_STATE_OPEN_LSEND)
		{
		    /* finished sending open response packet */
		    if (conn->pkt.sc_open_resp.ack == TRUE)
		    { 
			/* post receive for packet header */
			conn->state = CONN_STATE_CONNECTED;
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTED;
			mpi_errno = connection_post_recv_pkt(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**fail", NULL);
			    goto fn_exit;
			}
			
			mpi_errno = connection_post_sendq_req(conn);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
							     "**ch3|sock|openlsend", NULL);
			    goto fn_exit;
			}
		    }
		    else
		    {
			/* head-to-head connections - close this connection */
			conn->state = CONN_STATE_CLOSING;
			mpi_errno = MPIDU_Sock_post_close(conn->sock);
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							     "**sock_post_close", NULL);
			    goto fn_exit;
			}
		    }
		}
	    }

	    break;
	}
	    
	case MPIDU_SOCK_OP_ACCEPT:
	{
	    MPIDI_CH3I_Connection_t * conn;

	    mpi_errno = connection_alloc(&conn);
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|sock|accept", NULL);
		goto fn_exit;
	    }
	    mpi_errno = MPIDU_Sock_accept(MPIDI_CH3I_listener_conn->sock, sock_set, conn, &conn->sock);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						 "**ch3|sock|accept", NULL);
		connection_free(conn);
		goto fn_exit;
	    }
	    
	    conn->vc = NULL;
	    conn->state = CONN_STATE_OPEN_LRECV_PKT;
	    conn->send_active = NULL;
	    conn->recv_active = NULL;

	    mpi_errno = connection_post_recv_pkt(conn);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						 "**fail", NULL);
		goto fn_exit;
	    }
		
	    break;
	}
	    
	case MPIDU_SOCK_OP_CONNECT:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;

	    if (event->error != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(
		    event->error, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connfailed",
		    "**ch3|sock|connfailed %s %d", conn->vc->pg->id, conn->vc->pg_rank);
		goto fn_exit;
	    }

	    if (conn->state == CONN_STATE_CONNECTING)
	    {
		conn->state = CONN_STATE_OPEN_CSEND;
		MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_OPEN_REQ);
		conn->pkt.sc_open_req.pg_id_len = (int) strlen(MPIDI_Process.my_pg->id) + 1;
		conn->pkt.sc_open_req.pg_rank = MPIR_Process.comm_world->rank;

		connection_post_send_pkt_and_pgid(conn);
	    }
	    else
	    {
		/* CONN_STATE_CONNECT_ACCEPT */
		conn->state = CONN_STATE_OPEN_CSEND;
		MPIDI_Pkt_init(&conn->pkt, MPIDI_CH3I_PKT_SC_CONN_ACCEPT);
		/* pkt contains nothing */
		mpi_errno = connection_post_send_pkt(conn);
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
						     "**ch3|sock|scconnaccept", NULL);
		    goto fn_exit;
		}
	    }
		    
	    break;
	}
	    
	case MPIDU_SOCK_OP_CLOSE:
	{
	    MPIDI_CH3I_Connection_t * conn = (MPIDI_CH3I_Connection_t *) event->user_ptr;
		
	    /* If the conn pointer is NULL then the close was intentional */
	    if (conn != NULL)
	    {
		if (conn->state == CONN_STATE_CLOSING)
		{
		    MPIU_Assert(conn->send_active == NULL);
		    MPIU_Assert(conn->recv_active == NULL);
		    if (conn->vc != NULL)
		    {
			conn->vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
			conn->vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
			MPIDI_CH3U_Handle_connection(conn->vc, MPIDI_VC_EVENT_TERMINATED);
		    }
		}
		else
		{
		    MPIU_Assert(conn->state == CONN_STATE_LISTENING);
		    MPIDI_CH3I_listener_conn = NULL;
		    MPIDI_CH3I_listener_port = 0;
		    
		    /* MPIDI_CH3_Progress_signal_completion(); */
		    MPIDI_CH3I_progress_completion_count++;
		}
		
		conn->sock = MPIDU_SOCK_INVALID_SOCK;
		conn->state = CONN_STATE_CLOSED;
		connection_free(conn);
	    }
	    
	    break;
	}

	case MPIDU_SOCK_OP_WAKEUP:
	{
	    /* MPIDI_CH3_Progress_signal_completion(); */
	    MPIDI_CH3I_progress_completion_count++;
	    break;
	}
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_PROGRESS_HANDLE_SOCK_EVENT);
    return mpi_errno;
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
	    if (rc != 0)
	    { 
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
		goto impl_exit;
	    }
    
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


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_VC_post_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t * vc)
{
    char * key;
    char * val;
    char * val_p;
    int key_max_sz;
    int val_max_sz;
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port;
    int rc;
    int found;
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    MPIU_Assert(vc->ch.state == MPIDI_CH3I_VC_STATE_UNCONNECTED);
    
    vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;

    mpi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
    }
    val = MPIU_Malloc(val_max_sz);
    if (val == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_exit;
    }

    /* first lookup bizcard cache to see if bizcard is there. needed for spawn/connect/accept */
    mpi_errno = MPIDI_CH3I_Lookup_bizcard_cache(vc->pg->id, vc->pg_rank, val, 
                                                val_max_sz, &found);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    if (!found) {
        /* bizcard not found, get it from KVS space */

        mpi_errno = PMI_KVS_Get_key_length_max(&key_max_sz);
        if (mpi_errno != PMI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                             "**pmi_kvs_get_key_length_max", "**pmi_kvs_get_key_length_max %d", mpi_errno);
            goto fn_exit;
        }
        key = MPIU_Malloc(key_max_sz);
        if (key == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
            goto fn_exit;
        }

        rc = snprintf(key, key_max_sz, "P%d-businesscard", vc->pg_rank);
        if (rc < 0 || rc > key_max_sz)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
            goto fn_exit;
        }
        rc = PMI_KVS_Get(vc->pg->ch.kvs_name, key, val, val_max_sz);
        if (rc != PMI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get",
                                             "**pmi_kvs_get %d", rc);
            goto fn_exit;
        }

        MPIU_Free(key);
    }

    val_p = val;

    mpi_errno = MPIU_Str_get_string_arg(val, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description, MAX_HOST_DESCRIPTION_LEN);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_hostd", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_get_int_arg(val, MPIDI_CH3I_PORT_KEY, &port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_port", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
	return mpi_errno;
    }

    mpi_errno = connection_alloc(&conn);
    if (mpi_errno == MPI_SUCCESS)
    {
	mpi_errno = MPIDU_Sock_post_connect(sock_set, conn, host_description, port, &conn->sock);
	if (mpi_errno == MPI_SUCCESS)
	{
	    vc->ch.sock = conn->sock;
	    vc->ch.conn = conn;
	    conn->vc = vc;
	    conn->state = CONN_STATE_CONNECTING;
	    conn->send_active = NULL;
	    conn->recv_active = NULL;
	}
	else
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|postconnect",
		"**ch3|sock|postconnect %d %d %s", MPIR_Process.comm_world->rank, vc->pg_rank, val);

	    vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	    connection_free(conn);
	}
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connalloc", NULL);
    }

    MPIU_Free(val);

  fn_exit:
    MPIDI_DBG_PRINTF((60, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT);
    return mpi_errno;
}
/* end MPIDI_CH3I_VC_post_connect() */


#undef FUNCNAME
#define FUNCNAME  MPIDI_CH3I_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Connect_to_root(char * port_name, MPIDI_VC_t ** new_vc)
{
    /* Used in ch3_comm_connect to connect with the process calling ch3_comm_accept */
    char host_description[MAX_HOST_DESCRIPTION_LEN];
    int port;
    MPIDI_VC_t * vc;
    MPIDI_CH3I_Connection_t * conn;
    int mpi_errno = MPI_SUCCESS;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);

    mpi_errno = MPIU_Str_get_string_arg(port_name, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description, MAX_HOST_DESCRIPTION_LEN);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_hostd", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
	return mpi_errno;
    }
    mpi_errno = MPIU_Str_get_int_arg(port_name, MPIDI_CH3I_PORT_KEY, &port);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**argstr_port", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
	return mpi_errno;
    }
    
    
    vc = (MPIDI_VC_t *) MPIU_Malloc(sizeof(MPIDI_VC_t));
    if (vc == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_exit;
    }
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    MPIDI_VC_Init(vc, NULL, 0);
    vc->ch.sendq_head = NULL;
    vc->ch.sendq_tail = NULL;
    vc->ch.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    vc->ch.sock = MPIDU_SOCK_INVALID_SOCK;
    vc->ch.conn = NULL;
    
    mpi_errno = connection_alloc(&conn);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	goto fn_exit;
    }

    /* conn->pg_id is not used for this conection */

    mpi_errno = MPIDU_Sock_post_connect(sock_set, conn, host_description, port, &conn->sock);
    if (mpi_errno == MPI_SUCCESS)
    {
        vc->ch.sock = conn->sock;
        vc->ch.conn = conn;
        vc->ch.state = MPIDI_CH3I_VC_STATE_CONNECTING;
        conn->vc = vc;
        conn->state = CONN_STATE_CONNECT_ACCEPT;
        conn->send_active = NULL;
        conn->recv_active = NULL;
    }
    else
    {
	if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDU_SOCK_ERR_BAD_HOST)
        { 
            mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|badhost",
		"**ch3|sock|badhost %s %d %s", conn->pg_id, conn->vc->pg_rank, port_name);
        }
        else if (MPIR_ERR_GET_CLASS(mpi_errno) == MPIDU_SOCK_ERR_CONN_FAILED)
        { 
            mpi_errno = MPIR_Err_create_code(
		MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|sock|connrefused",
		"**ch3|sock|connrefused %s %d %s", conn->pg_id, conn->vc->pg_rank, port_name);
        }
        else
        {
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	}
        vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
        MPIU_Free(conn);
        goto fn_exit;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_CONNECT_TO_ROOT);
    return mpi_errno;
}
/* MPIDI_CH3I_Connect_to_root() */


#undef FUNCNAME
#define FUNCNAME connection_alloc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_alloc(MPIDI_CH3I_Connection_t ** connp)
{
    MPIDI_CH3I_Connection_t * conn = NULL;
    int id_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_ALLOC);
    conn = MPIU_Malloc(sizeof(MPIDI_CH3I_Connection_t));
    if (conn == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|sock|connallocfailed", NULL);
	goto fn_fail;
    }
    conn->pg_id = NULL;
    
    mpi_errno = PMI_Get_id_length_max(&id_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max",
					 "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_fail;
    }
    conn->pg_id = MPIU_Malloc(id_sz + 1);
    if (conn->pg_id == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
    }

    *connp = conn;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_ALLOC);
    return mpi_errno;

  fn_fail:
    if (conn != NULL)
    {
	if (conn->pg_id != NULL)
	{
	    MPIU_Free(conn->pg_id);
	}
	
	MPIU_Free(conn);
    }

    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME connection_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_free(MPIDI_CH3I_Connection_t * conn)
{
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_FREE);

    MPIU_Free(conn->pg_id);
    MPIU_Free(conn);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_FREE);
}


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
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = connection_send_fail(conn, mpi_errno);
	}
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SENDQ_REQ);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT);
    
    mpi_errno = MPIDU_Sock_post_write(conn->sock, &conn->pkt, sizeof(conn->pkt), sizeof(conn->pkt), NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = connection_send_fail(conn, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT);
    return mpi_errno;
}


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
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = connection_recv_fail(conn, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_RECV_PKT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME connection_post_send_pkt_and_pgid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void connection_post_send_pkt_and_pgid(MPIDI_CH3I_Connection_t * conn)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
    
    conn->iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) &conn->pkt;
    conn->iov[0].MPID_IOV_LEN = (int) sizeof(conn->pkt);

    conn->iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) MPIDI_Process.my_pg->id;
    conn->iov[1].MPID_IOV_LEN = (int) strlen(MPIDI_Process.my_pg->id) + 1;

    mpi_errno = MPIDU_Sock_post_writev(conn->sock, conn->iov, 2, NULL);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = connection_send_fail(conn, mpi_errno);
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_POST_SEND_PKT_AND_PGID);
}


#undef FUNCNAME
#define FUNCNAME connection_send_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_SEND_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_SEND_FAIL);

    mpi_errno = MPIR_Err_create_code(sock_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);

#   if 0
    {
	conn->state = CONN_STATE_FAILED;
	if (conn->vc != NULL)
	{
	    conn->vc->ch.state = MPIDI_CH3I_VC_STATE_FAILED;
	    MPIDI_CH3U_VC_send_failure(conn->vc, mpi_errno);
	}
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_SEND_FAIL);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME connection_recv_fail
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_CONNECTION_RECV_FAIL);

    MPIDI_FUNC_ENTER(MPID_STATE_CONNECTION_RECV_FAIL);

    mpi_errno = MPIR_Err_create_code(sock_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
    
    MPIDI_FUNC_EXIT(MPID_STATE_CONNECTION_RECV_FAIL);
    return mpi_errno;
}


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
	    iov[offset].MPID_IOV_BUF = (char *) iov[offset].MPID_IOV_BUF + nb;
	    iov[offset].MPID_IOV_LEN -= nb;
	    break;
	}
    }

    *iovp += offset;
    *countp -= offset;

    return (*countp == 0);
}
/* end adjust_iov() */
