/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/*
 * Description of the Bsend data structures.
 *
 * Bsend is buffered send; a buffer, provided by the user, is used to store
 * both the user's message and information that my be needed to send that
 * message.  In addition, space within that buffer must be allocated, so
 * additional information is required to manage that space allocation.  
 * In the following, the term "segment" denotes a fragment of the user buffer
 * that has been allocated either to free (unused) space or to a particular
 * user message.
 *
 * The following datastructures are used:
 *
 *  BsendMsg_t  - Describes a user message, including the values of tag
 *                and datatype (*could* be used incase the data is already 
 *                contiguous; see below)
 *  BsendData_t - Describes a segment of the user buffer.  This data structure
 *                contains a BsendMsg_t for segments that contain a user 
 *                message.  Each BsendData_t segment belongs to one of 
 *                three lists: avail (unused and free), active (currently
 *                sending) and pending (contains a user message that has
 *                not begun sending because of some resource limit, such
 *                as no more MPID requests available).
 *  BsendBuffer - This global structure contains pointers to the user buffer
 *                and the three lists, along with the size of the user buffer.
 *                Multithreaded versions contain a thread lock as well.
 *
 * Miscellaneous comments
 * By storing total_size along with "size available for messages", we avoid
 * any complexities associated with alignment, since we must ensure that each
 * BsendData_t structure is properly aligned (i.e., we can't simply
 * do (sizeof(BsendData_t) + size) to get total_size).
 */

/* Private structures for the bsend buffers */

/* BsendMsg is used to hold all of the message particulars in case
   a request is not currently available */
typedef struct {
    void         *msgbuf;
    int          count;
    MPI_Datatype dtype;
    int          tag;
    MPID_Comm    *comm_ptr;
    int          dest;
} BsendMsg_t;

/* BsendData describes a bsend request */
typedef struct BsendData {
    int              size;             /* size that is available for data */
    int              total_size;       /* total size of this segment */
    struct BsendData *next, *prev;
    MPID_Request     *request;
    BsendMsg_t       msg;
} BsendData_t;

/* BsendBuffer is the structure that describes the overall Bsend buffer */
static struct {
    void               *buffer;        /* Pointer to the begining of the user-
					  provided buffer */
    int                buffer_size;    /* Size of the user-provided buffer */
    BsendData_t        *avail;         /* Pointer to the first available block
					  of space */
    BsendData_t        *pending;       /* Pointer to the first message that
					  could not be sent because of a 
					  resource limit (e.g., no requests
					  available) */
    BsendData_t        *active;        /* Pointer to the first active (sending)
					  message */
#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_MULTIPLE
    MPID_Thread_lock_t bsend_lock;     /* Thread lock for bsend access */
#endif

} BsendBuffer = { 0, 0, 0, 0, 0 };

static int initialized = 0;   /* keep track of the first call to any
				 bsend routine */

/* Forward references */
static int MPIR_Bsend_release( void *, int );
static void MPIR_Bsend_retry_pending( void );
static void MPIR_Bsend_check_active ( void );
static BsendData_t *MPIR_Bsend_find_buffer( int );
static void MPIR_Bsend_take_buffer( BsendData_t *, int );

/*
 * Attach a buffer.  This checks for the error conditions and then
 * initialized the avail buffer.
 */    
int MPIR_Bsend_attach( void *buffer, int buffer_size )
{
    BsendData_t *p;

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    if (BsendBuffer.buffer) {
		return MPIR_Err_create_code( MPI_ERR_BUFFER, 
					     "**bufexists", 0 );
	    }
	    if (buffer_size < MPI_BSEND_OVERHEAD) {
		return MPIR_Err_create_code( MPI_ERR_OTHER, 
			     "**bsendbufsmall", "**bsendbufsmall %d %d",
			     buffer_size, MPI_BSEND_OVERHEAD );
	    }
	}
	MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (!initialized) {
	initialized = 1;
	/* FIXME : add callback to finalize handler */
    }

    BsendBuffer.buffer	    = buffer;
    BsendBuffer.buffer_size = buffer_size;
    BsendBuffer.avail	    = buffer;
    BsendBuffer.pending	    = 0;
    BsendBuffer.active	    = 0;
#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_MULTIPLE
    MPID_Thread_initlock( BsendBuffer.bsend_lock );
#endif

    /* Set the first block */
    p		  = (BsendData_t *)buffer;
    p->size	  = buffer_size - sizeof(BsendData_t);
    p->total_size = buffer_size;
    p->next	  = p->prev = 0;

    return MPI_SUCCESS;
}

/* 
 * Detach a buffer.  This routine must wait until any pending bsends 
 * are complete.
 */
int MPIR_Bsend_detach( void *p, int *size )
{
    if (BsendBuffer.pending) {
	return MPIR_Err_create_code( MPI_ERR_OTHER, "**notimpl", 0 );
    }
    if (BsendBuffer.active) {
	/* Loop through each active element and wait on it */
	BsendData_t *p = BsendBuffer.active;
	
	while (p) {
	    MPI_Request r = p->request->handle;
	    NMPI_Wait( &r, MPI_STATUS_IGNORE );
	    p = p->next;
	}
    }

    *size = BsendBuffer.buffer_size;
    BsendBuffer.buffer = 0;
    BsendBuffer.avail  = 0;

    return MPI_SUCCESS;
}

/*
 * Initiate an ibsend.  We'll used this for Bsend as well.
 */
int MPIR_Bsend_isend( void *buf, int count, MPI_Datatype dtype, 
		      int dest, int tag, MPID_Comm *comm_ptr, 
		      MPID_Request **request  )
{
    BsendData_t *p;
    int packsize, mpi_errno, pass;

    /* Find a free segment and copy the data into it.  If we could 
       have, we would already have used tBsend to send the message with
       no copying.
    */

    (void)NMPI_Pack_size( count, dtype, comm_ptr->handle, &packsize );

    for (pass = 0; pass < 2; pass++) {
	MPID_Thread_lock( &BsendBuffer.bsend_lock );
	
	p = MPIR_Bsend_find_buffer( packsize );
	p = BsendBuffer.avail;
	while (p) {
	    if (p->size <= packsize) { 
		/* Found a segment */
		
		/* Pack the data into the buffer */
		p->msg.count = 0;
		(void)NMPI_Pack( buf, count, dtype, p->msg.msgbuf, packsize, 
				 &p->msg.count, comm_ptr->handle );
		/* Try to send the message */
		mpi_errno = MPID_Send(p->msg.msgbuf, p->msg.count, MPI_PACKED, 
				      dest, tag, comm_ptr,
				      MPID_CONTEXT_INTRA_PT2PT, &p->request );
		/* If the error is "request not available", put this on the
		   pending list */
		/* FIXME: NOT YET DONE */
		if (mpi_errno == -1) {  /* -1 is a temporary place holder for
					   request-not-available */
		    p->msg.dtype     = MPI_PACKED;
		    p->msg.tag       = tag;
		    p->msg.comm_ptr  = comm_ptr;
		    p->msg.dest      = dest;
		    
		    /* FIXME: Still need to add to pending list */
		    
		    /* ibsend has a problem.  
		       Use a generalized request here? */
		}
		else if (p->request) {
		    /* Only remove this block from the avail list if the 
		       message has not been sent (MPID_Send may have already 
		       sent the data, in which case it returned a null
		       request) */
		    MPIR_Bsend_take_buffer( p, p->msg.count );
		    *request = p->request;
		}
		else {
		    *request = 0;
		}
		break;
	    }
	    p = p->next;
	}
	if (p && pass == 2) break;
	/* Try to complete some pending bsends */
	MPIR_Bsend_check_active( );
	/* Give priority to any pending operations */
	MPIR_Bsend_retry_pending( );
    }
    MPID_Thread_unlock( &BsendBuffer.bsend_lock );
    
    if (!p) {
	/* Return error for no buffer space found */
	return 1;
    }
    else {
	return MPI_SUCCESS;
    }
}

/*
 * The following routines are used to manage the allocation of bsend segments
 * in the user buffer.  These routines handle, for example, merging segments
 * when an active segment that is adjacent to a free segment becomes free.
 *
 */

/* Add block p to the free list. Merge into adjacent blocks */
static void MPIR_Bsend_free_segment( BsendData_t *p )
{
    BsendData_t *prev = p->prev, *avail = BsendBuffer.avail;
    int         inserted = 0;

    /* Remove the segment from the free list */
    if (prev) {
	prev->next = p->next;
    }
    else {
	BsendBuffer.active = p->next;
    }
    if (p->next) {
	p->next->prev = prev;
    }

    /* Merge into the avail list */
    while (avail) {
	if ((char *)avail + avail->total_size == (char *)p) {
	    /* Add p to avail, set p to this block and continue through
	       the loop to catch p+size == next avail */
	    avail->total_size += p->total_size;
	    p = avail;
	    inserted = 1;
	    /* No break */
	}
	else if ((char *)p + p->total_size == (char *)avail) {
	    /* Exact fit to the next entry.  Replace that entry 
	       with this one */
	    p->total_size += avail->total_size;
	    p->prev	   = avail->prev;
	    p->next	   = avail->next;
	    p->size        = p->total_size - sizeof(BsendData_t);
	    if (!p->prev) {
		BsendBuffer.avail = p;
	    }
	    break;
	}
	else if (p < avail) {
	    BsendData_t *prev = avail->prev;
	    if (inserted) break;   /* Exit if already inserted (top case) */
	    /* Insert p before avail */
	    p->next = avail;
	    p->prev = prev;
	    avail->prev = p;
	    if (prev) 
		prev->next = p;
	    else
		BsendBuffer.avail = p;
	    break;
	}
	avail = avail->next;
    }
}

/* 
 * The following routine tests for completion of active sends and 
 * frees the related storage
 *
 */
static void MPIR_Bsend_check_active( void )
{
    BsendData_t *active = BsendBuffer.active, *next_active;

    while (active) {
	MPI_Request r = active->request->handle;
	int         flag;
	
	next_active = active->next;
	NMPI_Test( &r, &flag, MPI_STATUS_IGNORE );
	if (flag) {
	    /* We're done.  Remove this segment */
	    MPIR_Bsend_free_segment( active );
	}
	active = next_active;
    }
}

/* 
 * For each pending item (that is, items that we couldn't even start sending),
 * try to get them going.  FIXME
 */
static void MPIR_Bsend_retry_pending( void )
{
    BsendData_t *pending = BsendBuffer.pending, *next_pending;

    while (pending) {
	next_pending = pending->next;
	/* Retry sending this item */
	/* FIXME */
	pending = next_pending;
    }
}

/* 
 * Find a slot in the avail buffer that can hold size bytes.  Does *not*
 * remove the slot from the avail buffer (see MPIR_Bsend_take_buffer) 
 */
static BsendData_t *MPIR_Bsend_find_buffer( int size )
{
    BsendData_t *p = BsendBuffer.avail;

    while (p) {
	if (p->size <= size) { 
	    return p;
	}
    }
    return 0;
}

/*
 * Carve off size bytes from buffer p and leave the remainder
 * on the avail list.  Handle the head/tail cases. 
 * If there isn't enough left of p, remove the entire segment from
 * the avail list.
 */
static void MPIR_Bsend_take_buffer( BsendData_t *p, int size  )
{
    BsendData_t *prev = p->prev, *bnext;
    int         newsize;
    
    /* Add p to the active list; save the next pointer */
    /* Question: this adds to the head.  Do we want to add to
       the tail? */
    bnext              = p->next;
    p->next            = BsendBuffer.active;
    BsendBuffer.active = p;
    
    /* shorten the segment.  If there isn't enough space, 
       just remove it from the avail list */
    
    newsize = p->size - p->msg.count - sizeof(BsendData_t);
    /* Round down to a multiple of 8 to preserve alignment */
    newsize -= (newsize & 0x7);
    
    if (newsize < 0) {
	/* The entire block goes into the active list; 
	   link around it */
	if (prev) {
	    prev->next = bnext;
	}
	else {
	    BsendBuffer.avail = bnext;
	}
	if (bnext) 
	    bnext->prev = prev;
    }
    else {
	p          += (p->total_size - newsize);
	p->total_size = 0;
	p->size    = newsize + sizeof(BsendData_t);
	p->next    = bnext;
	if (prev) {
	    prev->next = p;
	}
	else {
	    BsendBuffer.avail = p;
	}
	if (bnext) {
	    bnext->prev = p;
	}
    }
}
