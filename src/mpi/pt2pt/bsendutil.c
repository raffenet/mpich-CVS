/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

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
    struct BsendData *next, *prev;
    MPID_Request     *request;
    BsendMsg_t       msg;
} BsendData_t;

/* BsendBuffer is the structure that describes the overall Bsend buffer */
static struct {
    void               *buffer;        /* Pointer to the begining of the user-
					  provided buffer */
    int                size;           /* Size of the user-provided buffer */
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

/* Forward references */
static int MPIR_Bsend_release( void *, int );
    
int MPIR_Bsend_attach( void *buffer, int size )
{
    BsendData_t *p;
    if (BsendBuffer.buffer) {
	return MPIR_Err_create_code( MPI_ERR_BUFFER, "**bufexists", 0 );
    }
    if (size < MPI_BSEND_OVERHEAD) {
	return MPIR_Err_create_code( MPI_ERR_OTHER, 
			     "**bsendbufsmall", "**bsendbufsmall %d %d",
			     size, MPI_BSEND_OVERHEAD );
    }

    BsendBuffer.buffer  = buffer;
    BsendBuffer.size    = size;
    BsendBuffer.avail   = buffer;
    BsendBuffer.pending = 0;
    BsendBuffer.active  = 0;

    /* Set the first block */
    p       = (BsendData_t *)buffer;
    p->size = size - sizeof(BsendData_t);
    p->next = p->prev = 0;

    return MPI_SUCCESS;
}

/* This routine must wait until any pending bsends are complete */
int MPIR_Bsend_detach( void *p, int *size )
{
    if (BsendBuffer.pending) {
	return MPIR_Err_create_code( MPI_ERR_OTHER, "**notimpl", 0 );
    }
    *size = BsendBuffer.size;
    BsendBuffer.buffer = 0;
    BsendBuffer.avail  = 0;

    return MPI_SUCCESS;
}

int MPIR_Bsend_isend( void *buf, int count, MPI_Datatype dtype, 
		      int dest, int tag, MPID_Comm *comm_ptr, 
		      MPI_Request *request  )
{
    BsendData_t *p;
    int packsize, mpi_errno;

    /* Find a free segment and copy the data into it.  If we could 
       have, we would already have used tBsend to send the message with
       no copying */
    (void)PMPI_Pack_size( count, dtype, comm_ptr->handle, &packsize );
    MPID_Thread_lock( &BsendBuffer.bsend_lock );
    p = BsendBuffer.avail;
    while (p) {
	if (p->size <= packsize) { 
	    BsendData_t *prev = p->prev, *bnext;
	    int         newsize, nblocks;

	    /* Found a segment */

	    /* Pack the data into the buffer */
	    p->msg.count = 0;
	    (void)PMPI_Pack( buf, count, dtype, p->msg.msgbuf, packsize, 
			     &p->msg.count, comm_ptr->handle );
	    /* Try to send the message */
	    mpi_errno = MPID_Send(p->msg.msgbuf, p->msg.count, MPI_PACKED, 
				  dest, tag, comm_ptr,
				  MPID_CONTEXT_INTRA_PT2PT, &p->request );
	    /* If the error is "request not available", put this on the
	       pending list */
	    /* FIXME: NOT YET DONE */
	    p->msg.dtype     = MPI_PACKED;
	    p->msg.tag       = tag;
	    p->msg.comm_ptr  = comm_ptr;
	    p->msg.dest      = dest;
	    

	    /* shorten the segment.  If there isn't enough space, 
	       just remove it from the avail list */
	    
	    /* Add to the active list; save the next pointer */
	    bnext              = p->next;
	    p->next            = BsendBuffer.active;
	    BsendBuffer.active = p;

	    newsize = p->size - packsize - sizeof(BsendData_t);

	    /* We must preserve alignment on the BsendData_t structure. */
	    nblocks = (p->msg.count + sizeof(BsendData_t) - 1) /
		    sizeof(BsendData_t);
	    newsize = p->size - (nblocks + 1) * sizeof(BsendData_t);
	    if (newsize < 0) {
		prev->next = bnext;
	    }
	    else {
		p          += nblocks * sizeof(BsendData_t);
		p->size    = newsize + sizeof(BsendData_t);
		p->next    = bnext;
		prev->next = p;
	    }
	    break;
	}
	p = p->next;
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

/* This routine is called to release a buffer slot.
   It must try to merge with the next and the previous buffer 
*/
static int MPIR_Bsend_release( void *buf, int size )
{
    BsendData_t *p = BsendBuffer.avail;
    char *bufaddr = (char *)buf;

    if (buf == BsendBuffer.buffer) {
	/* Special case of buffer at the beginning */
	/* ....; */
	/* Include merge with the next one */
	return 0;
    }

    /* Otherwise, try to find the location within the buffer */
    while (p) {
	char *p_addr = (char *)p;
	/* Is this at the END of this block */
	if (p_addr + p->size + sizeof(BsendData_t) == bufaddr) {
	    BsendData_t *pnext = p->next;
	    char *pnext_addr = (char *)pnext;
	    p->size += size;
	    /* Is this new block at the beginning of the next one */
	    if (p_addr + p->size == pnext_addr) {
		p->next = pnext->next;
		p->size += pnext->size + sizeof(BsendData_t);
		pnext->prev = p;
	    }
	}
    }
    return 0;
}
