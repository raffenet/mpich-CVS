/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
#define mpig_recvq_lock() MPID_Thread_lock(&mpig_process.recvq_mutex)
#define mpig_recvq_unlock() MPID_Thread_unlock(&mpig_process.recvq_mutex)
#else
#define mpig_recvq_lock()
#define mpig_recvq_unlock()
#endif

/* Head and tail pointers for the posted and unexpected receive queues */
MPIG_STATIC MPID_Request * mpig_recvq_posted_head;
MPIG_STATIC MPID_Request * mpig_recvq_posted_tail;
MPIG_STATIC MPID_Request * mpig_recvq_unexp_head;
MPIG_STATIC MPID_Request * mpig_recvq_unexp_tail;


/*
 * mpig_recvq_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_init()
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_recvq_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    
    mpig_recvq_posted_head = NULL;
    mpig_recvq_posted_tail = NULL;
    mpig_recvq_unexp_head = NULL;
    mpig_recvq_unexp_tail = NULL;

    MPIG_DBG_PRINTF((10, FCNAME, "exiting (mpi_errno=%d)", mpi_errno));
    return mpi_errno;
}


/*
 * mpig_recvq_find_unexp()
 *
 * Find a matching request in the unexpected queue and return a pointer to the request object; otherwise, return NULL.
 *
 * NOTE: the returned request is locked.  The calling routine must unlock it.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_find_unexp
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_find_unexp(int rank, int tag, int ctx)
{
    MPID_Request * rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_find_unexp);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_find_unexp);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_recvq_lock();
    {
	const int rank_mask = (rank == MPI_ANY_SOURCE) ? 0 : ~0;;
	const int tag_mask = (tag == MPI_ANY_TAG) ? 0 : ~0;

	rreq = mpig_recvq_unexp_head;
	while(rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
		
	    mpig_request_get_envelope(rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    
	    if (rreq_ctx == ctx && (rreq_rank & rank_mask) == (rank & rank_mask) && (rreq_tag & tag_mask) == (tag & tag_mask))
	    {
		MPIG_DBG_PRINTF((15, FCNAME, "MATCHED req=0x%08x, reqp=%p, rank=%d, tag=%d, ctx=%d", rreq->handle, rreq,
				 rreq_rank, rreq_tag, rreq_ctx));
		mpig_request_lock(rreq);
		break;
	    }
	    else
	    {
		MPIG_DBG_PRINTF((15, FCNAME, "skipping req=0x%08x, reqp=%p, rank=%d, tag=%d, ctx=%d", rreq->handle, rreq,
				 rreq_rank, rreq_tag, rreq_ctx));
	    }
	    
	    rreq = rreq->dev.next;
	}
    }
    mpig_recvq_unlock();

    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_find_unexp);
    return rreq;
}
/* mpig_recvq_find_unexp() */


/*
 * mpig_recvq_deq_sent_unexp_sreq()
 *
 * Find a matching request in the unexpected queue and dequeue it.  Return a pointer to the request object if a matching request
 * was found; otherwise return NULL.  The primary purpose of this routine is to support canceling an already received message.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp_sreq
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_deq_unexp_sreq(int rank, int tag, int ctx, MPI_Request sreq_id)
{
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPID_Request * cur_rreq;
    MPID_Request * matching_prev_rreq;
    MPID_Request * matching_cur_rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_sreq);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_sreq);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    matching_prev_rreq = NULL;
    matching_cur_rreq = NULL;
    prev_rreq = NULL;
    
    mpig_recvq_lock();
    {
	cur_rreq = mpig_recvq_unexp_head;
	while(cur_rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
		
	    mpig_request_get_envelope(cur_rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    if (rreq_ctx == ctx && rreq_rank == rank && rreq_tag == tag)
	    {
		matching_prev_rreq = prev_rreq;
		matching_cur_rreq = cur_rreq;
	    }
	    
	    prev_rreq = cur_rreq;
	    cur_rreq = cur_rreq->dev.next;
	}

	if (matching_cur_rreq != NULL)
	{
	    if (matching_prev_rreq != NULL)
	    {
		matching_prev_rreq->dev.next = matching_cur_rreq->dev.next;
	    }
	    else
	    {
		mpig_recvq_unexp_head = matching_cur_rreq->dev.next;
	    }
	
	    if (matching_cur_rreq->dev.next == NULL)
	    {
		mpig_recvq_unexp_tail = matching_prev_rreq;
	    }

	    rreq = matching_cur_rreq;
	    rreq->dev.next = NULL;
	}
	else
	{
	    rreq = NULL;
	}
    }
    mpig_recvq_unlock();

    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp_sreq);
    return rreq;
}
/* mpig_recvq_deq_unexp_sreq() */


/*
 * mpig_recvq_deq_unexp_or_enq_posted()
 *
 * Find a matching request in the unexpected queue and dequeue it; otherwise allocate a new request and enqueue it in the posted
 * queue.  This is an atomic operation.
 *
 * NOTE: the returned request is locked.  The calling routine must unlock it.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp_or_enq_posted
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_deq_unexp_or_enq_posted(int rank, int tag, int ctx, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int lock_held = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    MPIG_DBG_PRINTF((15, FCNAME, "searching for rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
    
    lock_held = TRUE;
    mpig_recvq_lock();
    {
	const int rank_mask = (rank == MPI_ANY_SOURCE) ? 0 : ~0;;
	const int tag_mask = (tag == MPI_ANY_TAG) ? 0 : ~0;;
	    
	prev_rreq = NULL;
	rreq = mpig_recvq_unexp_head;
	while(rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
		
	    mpig_request_get_envelope(rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    
	    if (rreq_ctx == ctx && (rreq_rank & rank_mask) == (rank & rank_mask) && (rreq_tag & tag_mask) == (tag & tag_mask))
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = rreq->dev.next;
		}
		else
		{
		    mpig_recvq_unexp_head = rreq->dev.next;
		}
		if (rreq->dev.next == NULL)
		{
		    mpig_recvq_unexp_tail = prev_rreq;
		}
		MPIG_DBG_PRINTF((15, FCNAME, "MATCHED req=0x%08x, reqp=%p, rank=%d, tag=%d, ctx=%d", rreq->handle, rreq,
				 rreq_rank, rreq_tag, rreq_ctx));
		found = TRUE;
		goto lock_exit;
	    }
	    else
	    {
		MPIG_DBG_PRINTF((15, FCNAME, "skipping req=0x%08x, reqp=%p, rank=%d, tag=%d, ctx=%d", rreq->handle, rreq,
				 rreq_rank, rreq_tag, rreq_ctx));
	    }

	    prev_rreq = rreq;
	    rreq = rreq->dev.next;
	}

	/* A matching request was not found in the unexpected queue, so we need to allocate a new request and add it to the
	   posted queue */
	rreq = mpig_request_create();
	if (rreq == NULL) goto fn_fail;

	/* the request lock is acquired before inserting the req into the queue so that another thread doesn't dequeue it and
	   attempt to use it before the calling routine has a chance to initialized it */
	mpig_request_lock(rreq);
	
	if (mpig_recvq_posted_tail != NULL)
	{
	    mpig_recvq_posted_tail->dev.next = rreq;
	}
	else
	{
	    mpig_recvq_posted_head = rreq;
	}
	mpig_recvq_posted_tail = rreq;
	
	found = FALSE;
	
      lock_exit: ;
    }
    mpig_recvq_unlock();
    lock_held = FALSE;

    rreq->dev.next = NULL;

    if (found)
    {
	/* wait until the request has been initialized */
	mpig_request_lock(rreq);
    }

    *foundp = found;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    return rreq;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (lock_held)
    {
	mpig_recvq_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_deq_unexp_or_enq_posted() */

/*
 * mpig_recvq_deq_posted_rreq()
 *
 * Given a pointer to a reuqest object, attempt to find that request in the posted queue and dequeue it.  Return TRUE if the
 * request was located and dequeued, or FALSE if the request was not found.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_posted_rreq
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_recvq_deq_posted_rreq(MPID_Request * rreq)
{
    int found;
    MPID_Request * cur_rreq;
    MPID_Request * prev_rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_rreq);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_rreq);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    
    found = FALSE;
    prev_rreq = NULL;
    
    mpig_recvq_lock();
    {
	cur_rreq = mpig_recvq_posted_head;
	while (cur_rreq != NULL)
	{
	    if (cur_rreq == rreq)
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = cur_rreq->dev.next;
		}
		else
		{
		    mpig_recvq_posted_head = cur_rreq->dev.next;
		}
		if (cur_rreq->dev.next == NULL)
		{
		    mpig_recvq_posted_tail = prev_rreq;
		}
	    
		cur_rreq->dev.next = NULL;
		found = TRUE;
		break;
	    }
	    
	    prev_rreq = cur_rreq;
	    cur_rreq = cur_rreq->dev.next;
	}
    }
    mpig_recvq_unlock();

    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_posted_rreq);
    return found;
}
/* mpig_recvq_deq_posted_rreq */


/*
 * mpig_recvq_deq_posted_or_enq_unexp()
 *
 * Find a matching request in the posted queue and dequeue it; otherwise allocate a new request and enqueue it in the expected
 * queue.  This is an atomic operation.  
 *
 * NOTE: the returned request is locked.  The calling routine must unlock it.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_posted_or_enq_unexp
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_deq_posted_or_enq_unexp(int rank, int tag, int ctx, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int lock_held = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));
    MPIG_DBG_PRINTF((15, FCNAME, "searching for rank=%d, tag=%d, ctx=%d", rank, tag, ctx));

    mpig_recvq_lock();
    lock_held = TRUE;
    {
	prev_rreq = NULL;
	rreq = mpig_recvq_posted_head;
	while(rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
	    int rank_mask;
	    int tag_mask;
	    
	    mpig_request_get_envelope(rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    rank_mask = (rreq_rank == MPI_ANY_SOURCE) ? 0 : ~0;
	    tag_mask = (rreq_tag == MPI_ANY_TAG) ? 0 : ~0;
		
	    if (rreq_ctx == ctx && (rreq_rank & rank_mask) == (rank & rank_mask) && (rreq_tag & tag_mask) == (tag & tag_mask))
	    {
		if (prev_rreq != NULL)
		{
		    prev_rreq->dev.next = rreq->dev.next;
		}
		else
		{
		    mpig_recvq_posted_head = rreq->dev.next;
		}
		if (rreq->dev.next == NULL)
		{
		    mpig_recvq_posted_tail = prev_rreq;
		}
		
		MPIG_DBG_PRINTF((15, FCNAME, "MATCHED req=0x%08x, reqp=%p, rank=%d, tag=%d, ctx=%d", rreq->handle, rreq,
				     rreq_rank, rreq_tag, rreq_ctx));
		found = TRUE;
		goto lock_exit;
	    }
	    else
	    {
		MPIG_DBG_PRINTF((15, FCNAME, "skipping req=0x%08x, reqp=%p, rank=%d, tag=%d, ctx=%d", rreq->handle, rreq,
				 rreq_rank, rreq_tag, rreq_ctx));
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->dev.next;
	}
	
	/* A matching request was not found in the posted queue, so we need to allocate a new request and add it to the
	   unexpected queue */
	rreq = mpig_request_create();
	if (rreq == NULL) goto fn_fail;

	/* the request lock is acquired before inserting the req into the queue so that another thread doesn't dequeue it and
	   attempt to use it before the calling routine has a chance to initialized it */
	mpig_request_lock(rreq);
	
	if (mpig_recvq_unexp_tail != NULL)
	{
	    mpig_recvq_unexp_tail->dev.next = rreq;
	}
	else
	{
	    mpig_recvq_unexp_head = rreq;
	}
	
	mpig_recvq_unexp_tail = rreq;
        
	found = FALSE;
	
      lock_exit: ;
    }
    mpig_recvq_unlock();
    lock_held=FALSE;
    
    rreq->dev.next = NULL;
    
    if (found)
    { 
	/* wait until the request has been initialized */
	mpig_request_lock(rreq);
    }

    *foundp = found;

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    return rreq;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (lock_held)
    {
	mpig_recvq_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_deq_posted_or_enq_unexp() */
