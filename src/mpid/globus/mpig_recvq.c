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
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_find_unexp
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_find_unexp(mpig_rank_t rank, mpig_tag_t tag, mpig_ctx_t ctx)
{
    MPID_Request * rreq;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_find_unexp);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_find_unexp);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_recvq_lock();
    {
	if (tag != MPI_ANY_TAG && rank != MPI_ANY_SOURCE)
	{
	    rreq = mpig_recvq_unexp_head;
	    while(rreq != NULL)
	    {
		if (mpig_envelope_equal_tuple(&rreq->dev.envl, rank, tag, ctx))
		{
		    mpig_request_add_ref(rreq);
		    break;
		}
	    
		rreq = rreq->dev.next;
	    }
	}
	else
	{
	    mpig_envelope_t envl_val;
	    mpig_envelope_t envl_mask;

	    envl_val.ctx = ctx;
	    envl_mask.ctx = ~0;
	    if (tag == MPI_ANY_TAG)
	    {
		envl_val.tag = 0;
		envl_mask.tag = 0;
	    }
	    else
	    {
		envl_val.tag = tag;
		envl_mask.tag = ~0;
	    }
	    if (rank == MPI_ANY_SOURCE)
	    {
		envl_val.rank = 0;
		envl_mask.rank = 0;
	    }
	    else
	    {
		envl_val.rank = rank;
		envl_mask.rank = ~0;
	    }
	
	    rreq = mpig_recvq_unexp_head;
	    while (rreq != NULL)
	    {
		if (mpig_envelope_equal_masked(&rreq->dev.envl, &envl_val, &envl_mask))
		{
		    mpig_request_add_ref(rreq);
		    break;
		}
	    
		rreq = rreq->dev.next;
	    }
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
MPID_Request * mpig_recvq_deq_unexp_sreq(mpig_envelope_t * envl, MPI_Request sreq_id)
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
	    if (cur_rreq->dev.sreq_id == sreq_id && mpig_envelope_equal(&cur_rreq->dev.envl, envl))
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
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_unexp_or_enq_posted
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_deq_unexp_or_enq_posted(mpig_rank_t rank, mpig_tag_t tag, mpig_ctx_t ctx, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int lock_held = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_unexp_or_enq_posted);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    lock_held = TRUE;
    mpig_recvq_lock();
    {
	if (tag != MPI_ANY_TAG && rank != MPI_ANY_SOURCE)
	{
	    prev_rreq = NULL;
	    rreq = mpig_recvq_unexp_head;
	    while(rreq != NULL)
	    {
		if (mpig_envelope_equal_tuple(&rreq->dev.envl, rank, tag, ctx))
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
		    found = TRUE;
		    goto lock_exit;
		}
	    
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    }
	}
	else
	{
	    mpig_envelope_t envl_val;
	    mpig_envelope_t envl_mask;

	    envl_val.ctx = ctx;
	    envl_mask.ctx = ~0;
	    if (tag == MPI_ANY_TAG)
	    {
		envl_val.tag = 0;
		envl_mask.tag = 0;
	    }
	    else
	    {
		envl_val.tag = tag;
		envl_mask.tag = ~0;
	    }
	    if (rank == MPI_ANY_SOURCE)
	    {
		envl_val.rank = 0;
		envl_mask.rank = 0;
	    }
	    else
	    {
		envl_val.rank = rank;
		envl_mask.rank = ~0;
	    }
	
	    prev_rreq = NULL;
	    rreq = mpig_recvq_unexp_head;
	    while (rreq != NULL)
	    {
		if (mpig_envelope_equal_masked(&rreq->dev.envl, &envl_val, &envl_mask))
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
		    found = TRUE;
		    goto lock_exit;
		}
	    
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    }
	}


	/* A matching request was not found in the unexpected queue, so we need to allocate a new request and add it to the
	   posted queue */
	rreq = mpig_request_create();
	if (rreq == NULL) goto fn_fail;

	/* XXX: MT: mpig_request_lock(rreq); */
	rreq->kind = MPID_REQUEST_RECV;
	rreq->dev.envl.tag = tag;
	rreq->dev.envl.rank = rank;
	rreq->dev.envl.ctx = ctx;
	rreq->dev.next = NULL;
	
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

    if (found)
    {
	/* XXX: MT: mpig_request_lock(rreq); */
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
 * Locate a request in the posted queue and dequeue it, or allocate a new request and enqueue it in the unexpected queue
 */
#undef FUNCNAME
#define FUNCNAME mpig_recvq_deq_posted_or_enq_unexp
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
MPID_Request * mpig_recvq_deq_posted_or_enq_unexp(mpig_envelope_t * envl, int * foundp)
{
    int found;
    MPID_Request * rreq;
    MPID_Request * prev_rreq;
    int lock_held = FALSE;
    MPIG_STATE_DECL(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_recvq_deq_posted_or_enq_unexp);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

    mpig_recvq_lock();
    lock_held = TRUE;
    {
	if (envl->tag != MPI_ANY_TAG && envl->rank != MPI_ANY_SOURCE)
	{
	    prev_rreq = NULL;
	    rreq = mpig_recvq_posted_head;
	    while(rreq != NULL)
	    {
		if (mpig_envelope_equal(&rreq->dev.envl, envl))
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
		    found = TRUE;
		    goto lock_exit;
		}
	    
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    }
	}
	else
	{
	    mpig_envelope_t envl_val;
	    mpig_envelope_t envl_mask;

	    envl_val.ctx = envl->ctx;
	    envl_mask.ctx = ~0;
	    if (envl->tag == MPI_ANY_TAG)
	    {
		envl_val.tag = 0;
		envl_mask.tag = 0;
	    }
	    else
	    {
		envl_val.tag = envl->tag;
		envl_mask.tag = ~0;
	    }
	    if (envl->rank == MPI_ANY_SOURCE)
	    {
		envl_val.rank = 0;
		envl_mask.rank = 0;
	    }
	    else
	    {
		envl_val.rank = envl->rank;
		envl_mask.rank = ~0;
	    }
	
	    prev_rreq = NULL;
	    rreq = mpig_recvq_posted_head;
	    while (rreq != NULL)
	    {
		if (mpig_envelope_equal_masked(&rreq->dev.envl, &envl_val, &envl_mask))
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
		    found = TRUE;
		    goto lock_exit;
		}
	    
		prev_rreq = rreq;
		rreq = rreq->dev.next;
	    }
	}

	/* A matching request was not found in the posted queue, so we need to allocate a new request and add it to the
	   unexpected queue */
	rreq = mpig_request_create();
	if (rreq == NULL) goto fn_fail;

	/* XXX: MT: mpig_request_lock(rreq); */
	rreq->kind = MPID_REQUEST_RECV;
	rreq->dev.envl = *envl;
	rreq->dev.next = NULL;
	
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
    
    if (found)
    { 
	/* XXX: MT: mpig_request_lock(rreq); */
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
