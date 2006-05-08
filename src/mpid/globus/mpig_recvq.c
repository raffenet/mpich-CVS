/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

MPIG_STATIC globus_mutex_t mpig_recvq_mutex;

#define mpig_recvq_mutex_create()	globus_mutex_init(&mpig_recvq_mutex, NULL)
#define mpig_recvq_mutex_destroy()	globus_mutex_destroy(&mpig_recvq_mutex)
#define mpig_recvq_mutex_lock()							\
{										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_RECVQ,	\
		       "recvq - acquiring mutex"));				\
    globus_mutex_lock(&mpig_recvq_mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_RECVQ,	\
		       "recvq - mutex acquired"));				\
}
#define mpig_recvq_mutex_unlock()						\
{										\
    globus_mutex_unlock(&mpig_recvq_mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_RECVQ,	\
		       "recvq - mutex released"));				\
}

/* Head and tail pointers for the posted and unexpected receive queues */
MPIG_STATIC MPID_Request * mpig_recvq_posted_head;
MPIG_STATIC MPID_Request * mpig_recvq_posted_tail;
MPIG_STATIC MPID_Request * mpig_recvq_unexp_head;
MPIG_STATIC MPID_Request * mpig_recvq_unexp_tail;


/*
 * int mpig_recvq_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_init
void mpig_recvq_init(int * mpi_errno_p, bool_t * failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));

    *failed_p = FALSE;
    
    mpig_recvq_mutex_create();
    mpig_recvq_posted_head = NULL;
    mpig_recvq_posted_tail = NULL;
    mpig_recvq_unexp_head = NULL;
    mpig_recvq_unexp_tail = NULL;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_init);
    return;
}
/* mpig_recvq_init() */


/*
 * int mpig_recvq_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_finalize
void mpig_recvq_finalize(int * mpi_errno_p, bool_t * failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_destroy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));

    *failed_p = FALSE;
    
    mpig_recvq_posted_head = NULL;
    mpig_recvq_posted_tail = NULL;
    mpig_recvq_unexp_head = NULL;
    mpig_recvq_unexp_tail = NULL;
    mpig_recvq_mutex_destroy();

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: mpi_errno=0x%08x, failed=%s", *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_destroy);
    return;
}
/* mpig_recvq_finalize() */


/*
 * mpig_recvq_find_unexp()
 *
 * Find a matching request in the unexpected queue and return a pointer to the request object; otherwise, return NULL.
 *
 * NOTE: the mutex of the returned request is locked.  the calling routine must unlock it and should avoid calling any additional
 * receive queue routines until it has done so.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_find_unexp
MPID_Request * mpig_recvq_find_unexp(int rank, int tag, int ctx)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_find_unexp);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_find_unexp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));

    mpig_recvq_mutex_lock();
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
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
				   "MATCHED req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d",
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_rank, rreq_tag, rreq_ctx));
		/* MT-NOTE: the request mutex must be acquired before the recvq mutex is release.  if the recvq mutex were
		   released first, another thread could dequeue, acquire the request mutex, and destroy the request object before
		   this thread managed to acquire the request mutex. */
		mpig_request_mutex_lock(rreq);
		break;
	    }
	    else
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
				   "skipping req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d",
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_rank, rreq_tag, rreq_ctx));
	    }
	    
	    rreq = rreq->dev.next;
	}
    }
    mpig_recvq_mutex_unlock();

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: rank=%d, tag=%d, ctx=%d, rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT,
		       rank, tag, ctx, MPIG_HANDLE_VAL(rreq), (MPIG_PTR_CAST) rreq));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_find_unexp);
    return rreq;
}
/* mpig_recvq_find_unexp() */


/*
 * mpig_recvq_deq_unexp()
 *
 * Find a matching request in the unexpected queue and dequeue it.  Return a pointer to the request object if a matching request
 * was found; otherwise return NULL.  The primary purpose of this routine is to support canceling an already received but not
 * matched message.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp
MPID_Request * mpig_recvq_deq_unexp(int rank, int tag, int ctx, MPI_Request sreq_id)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    MPID_Request * cur_rreq;
    MPID_Request * matching_prev_rreq;
    MPID_Request * matching_cur_rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering"));

    matching_prev_rreq = NULL;
    matching_cur_rreq = NULL;
    prev_rreq = NULL;
    
    mpig_recvq_mutex_lock();
    {
	cur_rreq = mpig_recvq_unexp_head;
	while(cur_rreq != NULL)
	{
	    int rreq_rank;
	    int rreq_tag;
	    int rreq_ctx;
		
	    mpig_request_get_envelope(cur_rreq, &rreq_rank, &rreq_tag, &rreq_ctx);
	    if (rreq_ctx == ctx && rreq_rank == rank && rreq_tag == tag  && mpig_request_get_remote_req_id(cur_rreq) == sreq_id)
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
    mpig_recvq_mutex_unlock();

    /* If the request is still being initialized by another thread, wait for that process to complete */
    mpig_request_mutex_lock_conditional(rreq, (rreq != NULL));
	
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp);
    return rreq;
}
/* mpig_recvq_deq_unexp() */


/*
 * mpig_recvq_deq_unexp_rreq()
 *
 * Given a pointer to a reuqest object, attempt to find that request in the posted queue and dequeue it.  Return TRUE if the
 * request was located and dequeued, or FALSE if the request was not found.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp_rreq
bool_t mpig_recvq_deq_unexp_rreq(MPID_Request * rreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t found;
    MPID_Request * cur_rreq;
    MPID_Request * prev_rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_rreq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_rreq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, (MPIG_PTR_CAST) rreq));
    
    found = FALSE;
    prev_rreq = NULL;

    mpig_recvq_mutex_lock();
    {
	cur_rreq = mpig_recvq_unexp_head;
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
		    mpig_recvq_unexp_head = cur_rreq->dev.next;
		}
		if (cur_rreq->dev.next == NULL)
		{
		    mpig_recvq_unexp_tail = prev_rreq;
		}
	    
		cur_rreq->dev.next = NULL;
		found = TRUE;
		break;
	    }
	    
	    prev_rreq = cur_rreq;
	    cur_rreq = cur_rreq->dev.next;
	}
    }
    mpig_recvq_mutex_unlock();
	
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", found=%s",
		       rreq->handle, (MPIG_PTR_CAST) rreq, MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp_rreq);
    return found;
}
/* mpig_recvq_deq_unexp_rreq() */


/*
 * mpig_recvq_deq_posted_rreq()
 *
 * Given a pointer to a reuqest object, attempt to find that request in the posted queue and dequeue it.  Return TRUE if the
 * request was located and dequeued, or FALSE if the request was not found.
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_posted_rreq
bool_t mpig_recvq_deq_posted_rreq(MPID_Request * rreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t found;
    MPID_Request * cur_rreq;
    MPID_Request * prev_rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_rreq);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_rreq);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, (MPIG_PTR_CAST) rreq));
    
    found = FALSE;
    prev_rreq = NULL;
    
    mpig_recvq_mutex_lock();
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
    mpig_recvq_mutex_unlock();
	
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", found=%s",
		       rreq->handle, (MPIG_PTR_CAST) rreq, MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_posted_rreq);
    return found;
}
/* mpig_recvq_deq_posted_rreq() */


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
MPID_Request * mpig_recvq_deq_unexp_or_enq_posted(int rank, int tag, int ctx, int * foundp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int found = FALSE;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int lock_held = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
    
    lock_held = TRUE;
    mpig_recvq_mutex_lock();
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
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
				   "MATCHED UNEXP: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d",
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_rank, rreq_tag, rreq_ctx));
		
		found = TRUE;
		goto recvq_lock_exit;
	    }
	    else
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
				   "skipping: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d",
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_rank, rreq_tag, rreq_ctx));
	    }

	    prev_rreq = rreq;
	    rreq = rreq->dev.next;
	}

	/* A matching request was not found in the unexpected queue, so we need to allocate a new request and add it to the
	   posted queue */
	mpig_request_alloc(&rreq);
	if (rreq == NULL) goto fn_fail;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
			   "ENQUEUE POSTED: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d",
			   rreq->handle, (MPIG_PTR_CAST) rreq, rank, tag, ctx));
	
	/* the request lock is acquired before inserting the req into the queue so that another thread doesn't dequeue it and
	   attempt to use it before the calling routine has a chance to initialized it */
	mpig_request_mutex_lock(rreq);
	mpig_request_construct(rreq);
	mpig_request_set_envelope(rreq, rank, tag, ctx);

	if (mpig_recvq_posted_tail != NULL)
	{
	    mpig_recvq_posted_tail->dev.next = rreq;
	}
	else
	{
	    mpig_recvq_posted_head = rreq;
	}
	mpig_recvq_posted_tail = rreq;
	
      recvq_lock_exit:
	rreq->dev.next = NULL;
    }
    mpig_recvq_mutex_unlock();
    lock_held = FALSE;

    if (found)
    {
	/* wait until the request has been initialized */
	mpig_request_mutex_lock(rreq);
    }

    *foundp = found;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: rank=%d, tag=%d, ctx=%d, rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", found=%s",
		       rank, tag, ctx, rreq->handle, (MPIG_PTR_CAST) rreq, MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    return rreq;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (lock_held)
    {
	mpig_recvq_mutex_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_deq_unexp_or_enq_posted() */

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
MPID_Request * mpig_recvq_deq_posted_or_enq_unexp(int rank, int tag, int ctx, int * foundp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int found = FALSE;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int lock_held = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "entering: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));

    mpig_recvq_mutex_lock();
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
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
				   "MATCHED POSTED: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d, ctx=%d",
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_rank, rreq_tag, rreq_ctx));
		found = TRUE;
		goto recvq_lock_exit;
	    }
	    else
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
				   "skipping: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d" ",ctx=%d",
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_rank, rreq_tag, rreq_ctx));
	    }
	    
	    prev_rreq = rreq;
	    rreq = rreq->dev.next;
	}
	
	/* A matching request was not found in the posted queue, so we need to allocate a new request and add it to the
	   unexpected queue */
	mpig_request_alloc(&rreq);
	if (rreq == NULL) goto fn_fail;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_RECVQ,
			   "ENQUEDED UNEXP: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", rank=%d, tag=%d ,ctx=%d",
			   rreq->handle, (MPIG_PTR_CAST) rreq, rank, tag, ctx));
	
	/* the request lock is acquired before inserting the req into the queue so that another thread doesn't dequeue it and
	   attempt to use it before the calling routine has a chance to initialized it */
	mpig_request_mutex_lock(rreq);
	mpig_request_construct(rreq);
	mpig_request_set_envelope(rreq, rank, tag, ctx);
	
	if (mpig_recvq_unexp_tail != NULL)
	{
	    mpig_recvq_unexp_tail->dev.next = rreq;
	}
	else
	{
	    mpig_recvq_unexp_head = rreq;
	}
	
	mpig_recvq_unexp_tail = rreq;
        
      recvq_lock_exit:
	rreq->dev.next = NULL;
    }
    mpig_recvq_mutex_unlock();
    lock_held=FALSE;
    
    if (found)
    { 
	/* wait until the request has been initialized */
	mpig_request_mutex_lock(rreq);
    }

    *foundp = found;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_RECVQ,
		       "exiting: rank=%d, tag=%d, ctx=%d, rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", found=%s",
		       rank, tag, ctx, rreq->handle, (MPIG_PTR_CAST) rreq, MPIG_BOOL_STR(found)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    return rreq;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (lock_held)
    {
	mpig_recvq_mutex_unlock();
    }
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_recvq_deq_posted_or_enq_unexp() */
