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

/* #define DEBUG(a) a;fflush(stdout)  */
#define DEBUG(a) 
#define DEBUG1(a) 
/* #define DBG_PRINT_AVAIL */
/* #define DEBUG1(a) a;fflush(stdout)  */
/* #define DBG_PRINT_ARENA */

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
    int              total_size;       /* total size of this segment, 
					  including all headers */
    struct BsendData *next, *prev;
    BsendKind_t      kind;
    MPID_Request     *request;
    BsendMsg_t       msg;
    double           alignpad;         /* make sure that the struct shares
					  double alignment */
} BsendData_t;
#define BSENDDATA_HEADER_TRUE_SIZE (sizeof(BsendData_t) - sizeof(double))

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
static void MPIR_Bsend_retry_pending( void );
static void MPIR_Bsend_check_active ( void );
static BsendData_t *MPIR_Bsend_find_buffer( int );
static void MPIR_Bsend_take_buffer( BsendData_t *, int );
static int MPIR_Bsend_finalize( void * );

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
	MPIR_Add_finalize( MPIR_Bsend_finalize, (void *)0, 10 );
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
    p->size	  = buffer_size - BSENDDATA_HEADER_TRUE_SIZE;
    p->total_size = buffer_size;
    p->next	  = p->prev = 0;
    p->msg.msgbuf = (char *)p + BSENDDATA_HEADER_TRUE_SIZE;

    return MPI_SUCCESS;
}

/* 
 * Detach a buffer.  This routine must wait until any pending bsends 
 * are complete.
 */
int MPIR_Bsend_detach( void *bufferp, int *size )
{
    if (BsendBuffer.pending) {
	return MPIR_Err_create_code( MPI_ERR_OTHER, "**notimpl", 0 );
    }
    if (BsendBuffer.buffer == 0) {
	/* Error - detaching an already detached buffer */
	return MPIR_Err_create_code( MPI_ERR_OTHER, "**bsendnobuf", 0 );
    }
    if (BsendBuffer.active) {
	/* Loop through each active element and wait on it */
	BsendData_t *p = BsendBuffer.active;
	
	MPIR_Nest_incr();
	while (p) {
	    MPI_Request r = p->request->handle;
	    NMPI_Wait( &r, MPI_STATUS_IGNORE );
	    p = p->next;
	}
	MPIR_Nest_decr();
    }

    *(void **) bufferp  = BsendBuffer.buffer;
    *size = BsendBuffer.buffer_size;
    BsendBuffer.buffer  = 0;
    BsendBuffer.avail   = 0;
    BsendBuffer.active  = 0;
    BsendBuffer.pending = 0;

    return MPI_SUCCESS;
}

/*
 * Initiate an ibsend.  We'll used this for Bsend as well.
 */
int MPIR_Bsend_isend( void *buf, int count, MPI_Datatype dtype, 
		      int dest, int tag, MPID_Comm *comm_ptr, 
		      BsendKind_t kind, MPID_Request **request )
{
    BsendData_t *p;
    int packsize, mpi_errno, pass;

    /* Find a free segment and copy the data into it.  If we could 
       have, we would already have used tBsend to send the message with
       no copying.

       We may want to decide here whether we need to pack at all 
       or if we can just use (a memcpy) of the buffer.
    */

    MPIR_Nest_incr();

    /* We check the active buffer first.  This helps avoid storage 
       fragmentation */
    MPIR_Bsend_check_active();

    (void)NMPI_Pack_size( count, dtype, comm_ptr->handle, &packsize );

    DEBUG1(printf("looking for buffer of size %d\n", packsize));
    /*
     * Use two passes.  Each pass is the same; between the two passes,
     * attempt to complete any active requests, and start any pending
     * ones.  If the message can be initiated in the first pass,
     * do not perform the second pass.
     */
    MPID_Thread_lock( &BsendBuffer.bsend_lock );
    for (pass = 0; pass < 2; pass++) {
	
	p = MPIR_Bsend_find_buffer( packsize );
	if (p) {
	    DEBUG(printf("found buffer of size %d with address %x\n",packsize,p));
	    /* Found a segment */
	    
	    /* Pack the data into the buffer */
	    /* We may want to optimize for the special case of
	       either primative or contiguous types, and just
	       use memcpy and the provided datatype */
	    p->msg.count = 0;
	    (void)NMPI_Pack( buf, count, dtype, p->msg.msgbuf, packsize, 
			     &p->msg.count, comm_ptr->handle );
	    /* Try to send the message.  We must use MPID_Isend
	       because this call must not block */
	    mpi_errno = MPID_Isend(p->msg.msgbuf, p->msg.count, MPI_PACKED, 
				   dest, tag, comm_ptr,
				   MPID_CONTEXT_INTRA_PT2PT, &p->request );
	    if(mpi_errno) {
		printf ("Surprise! err = %d\n", mpi_errno );
	    }
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
		p->kind = kind;
	    }
	    else if (p->request) {
		DEBUG(printf("saving request %x in %x\n",p->request,p));
		/* Only remove this block from the avail list if the 
		   message has not been sent (MPID_Send may have already 
		   sent the data, in which case it returned a null
		   request) */
#if 0
		/* If the request is already complete, bypass the step
		   of saving the require and taking the buffer */
		if (p->request->cc_ptr == 0) {
		    mpi_errno = MPIR_Request_complete(p->request->handle, 
						      p->requestr, 
					      MPI_STATUS_IGNORE, &active_flag);
		}
#endif
		MPIR_Bsend_take_buffer( p, p->msg.count );
		p->kind  = kind;
		*request = p->request;
	    }
	    else {
		*request = 0;
	    }
	    break;
	}
	if (p && pass == 2) break;
	DEBUG(printf("Could not find storage, checking active\n" ));
	/* Try to complete some pending bsends */
	MPIR_Bsend_check_active( );
	/* Give priority to any pending operations */
	MPIR_Bsend_retry_pending( );
    }
    MPID_Thread_unlock( &BsendBuffer.bsend_lock );
    MPIR_Nest_decr();
    
    if (!p) {
	/* Return error for no buffer space found */
	return MPIR_Err_create_code( MPI_ERR_BUFFER, "**bufbsend", 
				     "**bufbsend %d %d", packsize, 
				     BsendBuffer.buffer_size );
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

/* Add block p to the free list. Merge into adjacent blocks.  Used only 
   within the check_active */
/* begin:nested */
static void MPIR_Bsend_free_segment( BsendData_t *p )
{
    BsendData_t *prev = p->prev, *avail = BsendBuffer.avail, *avail_prev;

    DEBUG1(printf("Freeing bsend segment at %x of size %d, next at %x\n",
		 p,p->size, ((char *)p)+p->total_size));

#ifdef DBG_PRINT_ARENA
    PRINTF( "At the begining of free_segment with size %d:\n", p->total_size );
    MPIR_Bsend_dump();
#endif    

    /* Remove the segment from the free list */
    if (prev) {
	DEBUG(printf("free segment is within active list\n"));
	prev->next = p->next;
    }
    else {
	/* p was at the head of the active list */
	DEBUG(printf("free segment is head of active list\n"));
	BsendBuffer.active = p->next;
	/* The next test sets the prev pointer to null */
    }
    if (p->next) {
	p->next->prev = prev;
    }

#ifdef DBG_PRINT_AVAIL_LIST
    {
	BsendData_t *a = BsendBuffer.avail;
	PRINTF( "Avail list is:\n" );
	while (a) {
	    PRINTF( "[%x] totalsize = %d(%x)\n", a, a->total_size, 
		   a->total_size );
	    a = a->next;
	}
    }
#endif
    /* Merge into the avail list */
    /* Find avail_prev, avail, such that p is between them.
       either may be null if p is at either end of the list */
    avail_prev = 0;
    while (avail) {
	if (avail > p) {
	    break;
	}
	avail_prev = avail;
	avail      = avail->next;
    }

    /* Try to merge p with the next block */
    if (avail) {
	if ((char *)p + p->total_size == (char *)avail) {
	    p->total_size += avail->total_size;
	    p->size       = p->total_size - BSENDDATA_HEADER_TRUE_SIZE;
	    p->next = avail->next;
	    if (avail->next) avail->next->prev = p;
	    avail = 0;
	}
	else {
	    p->next = avail;
	    avail->prev = p;
	}
    }
    else {
	p->next = 0;
    }
    /* Try to merge p with the previous block */
    if (avail_prev) {
	if ((char *)avail_prev + avail_prev->total_size == (char *)p) {
	    avail_prev->total_size += p->total_size;
	    avail_prev->size       = p->total_size - BSENDDATA_HEADER_TRUE_SIZE;
	    avail_prev->next = p->next;
	    if (p->next) p->next->prev = avail_prev;
	}
	else {
	    avail_prev->next = p;
	    p->prev          = avail_prev;
	}
    }
    else {
	/* p is the new head of the list */
	BsendBuffer.avail = p;
	p->prev           = 0;
    }
#ifdef DBG_PRINT_ARENA
    PRINTF( "At the end of free_segment:\n" );
    MPIR_Bsend_dump();
#endif    
}
/* end:nested */
/* 
 * The following routine tests for completion of active sends and 
 * frees the related storage
 *
 * To make it easier to identify the source of the request, we keep
 * track of the type of MPI routine (ibsend, bsend, or bsend_init/start)
 * that created the bsend entry.
 */
static void MPIR_Bsend_check_active( void )
{
    BsendData_t *active = BsendBuffer.active, *next_active;

    DEBUG(printf("Checking active starting at %x\n", active ));
    while (active) {
	MPI_Request r = active->request->handle;
	int         flag;
	
	next_active = active->next;
	NMPI_Test( &r, &flag, MPI_STATUS_IGNORE );
	if (flag) {
	    /* We're done.  Remove this segment */
	    DEBUG(printf("Removing segment %x\n", active ));
	    MPIR_Bsend_free_segment( active );
	}
	active = next_active;
	DEBUG(printf("Next active is %x\n",active));
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
	if (p->size >= size) { 
	    return p;
	}
	p = p->next;
    }
    return 0;
}

/* This is the minimum number of bytes that a segment must be able to
   hold. */
#define MIN_BUFFER_BLOCK 8
/*
 * Carve off size bytes from buffer p and leave the remainder
 * on the avail list.  Handle the head/tail cases. 
 * If there isn't enough left of p, remove the entire segment from
 * the avail list.
 */
static void MPIR_Bsend_take_buffer( BsendData_t *p, int size  )
{
    BsendData_t *prev;
    int         alloc_size;

    /* Compute the remaining size.  This must include any padding 
       that must be added to make the new block properly aligned */
    alloc_size = size;
    if (alloc_size & 0x7) 
	alloc_size += (8 - (alloc_size & 0x7));
    /* alloc_size is the amount of space (out of size) that we will 
       allocate for this buffer. */

#ifdef DBG_PRINT_ARENA
    PRINTF( "Taking %d bytes from a block with %d bytes\n", alloc_size, 
	    p->total_size );
#endif

    /* Is there enough space left to create a new block? */
    if (alloc_size + BSENDDATA_HEADER_TRUE_SIZE + MIN_BUFFER_BLOCK <= p->size) {
	/* Yes, the available space (p->size) is large enough to 
	   carve out a new block */
	BsendData_t *newp;
	
	DEBUG(printf("Breaking block into used and allocated at %x\n", p ));
	newp = (BsendData_t *)( (char *)p + BSENDDATA_HEADER_TRUE_SIZE + 
				alloc_size );
	newp->total_size = p->total_size - alloc_size - 
	    BSENDDATA_HEADER_TRUE_SIZE;
	newp->size = newp->total_size - BSENDDATA_HEADER_TRUE_SIZE;
	newp->msg.msgbuf = (char *)newp + BSENDDATA_HEADER_TRUE_SIZE;

	/* Insert this new block after p (we'll remove p from the avail list
	   next) */
	newp->next = p->next;
	newp->prev = p;
	if (p->next) {
	    p->next->prev = newp;
	}
	p->next       = newp;
	p->total_size = (char *)newp - (char*)p;
	p->size       = p->total_size - BSENDDATA_HEADER_TRUE_SIZE;

#ifdef DBG_PRINT_ARENA
	PRINTF( "broken blocks p (%d) and new (%d)\n",
		p->total_size, newp->total_size ); fflush(stdout);
#endif
    }

    /* Remove p from the avail list and add it to the active list */
    prev = p->prev;
    if (prev) {
	prev->next = p->next;
    }
    else {
	BsendBuffer.avail = p->next;
    }

    if (p->next) {
	p->next->prev = p->prev;
    }
	
    if (BsendBuffer.active) {
	BsendBuffer.active->prev = p;
    }
    p->next	       = BsendBuffer.active;
    p->prev	       = 0;
    BsendBuffer.active = p;

#ifdef DBG_PRINT_ARENA
    PRINTF( "At end of take buffer\n" );
    MPIR_Bsend_dump();
#endif
    DEBUG(printf("segment %x now head of active\n", p ));
}

/* Ignore p */
static int MPIR_Bsend_finalize( void *p )
{
    void *b;
    int  s;

    if (BsendBuffer.buffer) {
	/* Use detach to complete any communication */
	MPIR_Bsend_detach( &b, &s );
    }
    return 0;
}

#ifdef DBG_PRINT_ARENA
void MPIR_Bsend_dump( void )
{
    BsendData_t *a = BsendBuffer.avail;

    PRINTF( "Total size is %d\n", BsendBuffer.buffer_size );
    PRINTF( "Avail list is:\n" );
    while (a) {
	PRINTF( "[%x] totalsize = %d(%x)\n", a, a->total_size, 
		a->total_size );
	if (a == a->next) {
	    PRINTF( "@@@Corrupt list; avail block points at itself\n" );
	    break;
	}
	a = a->next;
    }

    PRINTF( "Active list is:\n" );
    a = BsendBuffer.active;
    while (a) {
	PRINTF( "[%x] totalsize = %d(%x)\n", a, a->total_size, 
		a->total_size );
	if (a == a->next) {
	    PRINTF( "@@@Corrupt list; active block points at itself\n" );
	    break;
	}
	a = a->next;
    }
    PRINTF( "end of list\n" );
    fflush( stdout );
}
#endif
