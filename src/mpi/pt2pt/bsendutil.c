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
    void *buf;
    int  count;
    MPI_Datatype dtype;
    int tag;
    MPI_Comm comm;
    int dest;
} BsendMsg_t;

/* BsendData describes a bsend request */
typedef struct BsendData {
    int size;                      /* size that is available for data */
    struct BsendData *next, *prev;
    MPID_Request *request;
    BsendMsg_t msg;
} BsendData_t;

/* BsendBuffer is the structure that describes the overall Bsend buffer */
static struct {
    void        *buffer;
    int         size;
    BsendData_t *avail;
    BsendData_t *pending;
#if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_MULTIPLE
    MPID_Thread_lock_t bsend_lock;     /* Thread lock for bsend access */
#endif

} BsendBuffer = { 0, 0, 0, 0 };
    
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

    BsendBuffer.buffer = buffer;
    BsendBuffer.size = size;
    BsendBuffer.avail = buffer;
    BsendBuffer.pending = 0;
    
    p = (BsendData_t *)buffer;
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

#ifdef FOO
int MPIR_Bsend_isend( void *buf, int count, MPI_Datatype dtype, 
		      int dest, int tag, MPID_Comm *comm_ptr, 
		      MPI_Request *request  )
{
    BsendData_t *p;
    int packsize;

    /* Find a free segment and copy the data into it.  If we could 
       have, we would already have used tBsend to send the message with
       no copying */
    (void)PMPI_Pack_size( count, dtype, &packsize );
    p = BsendBuffer.avail;
    while (p) {
	if (p->size <= packsize) { 
	    BsendData_t *prev = p->prev, *bnext;
	    int  newsize;

	    /* Found a segment */

	    /* Pack the data into the buffer */
	    (void)PMPI_Pack( buf, count, dtype, p->buffer, packsize, &position,
		       comm_ptr->handle );
	    /* Try to send the message */
	    mpi_errno = MPID_Send(p->buffer, packsize, MPI_PACKED, dest, tag, 
				  comm_ptr,
				  MPID_CONTEXT_INTRA_PT2PT, &p->request );
	    /* If the error is "request not available", put this on the
	       pending list */
	    /* NOT YET DONE */
	    
	    /* shorten the segment.  If there isn't enough space, 
	       just remove it from the avail list */
	    
	    /* Add to the active list; save the next pointer */
	    bnext = p->next;
	    p->next = BsendBuffer.active;
	    BsendBuffer.active = p;

	    newsize = p->size - packsize - sizeof(BsendData_t)
	    if (newsize < 0) {
		prev->next = bnext;
	    }
	    else {
		p = (BsendData_t *)(&p->buffer[packsize]);
		p->size = newsize;
		p->next = bnext;
		prev->next = p;
	    }
	    break;
	}
	p = p->next;
    }
    
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
int MPIR_Bsend_release( void *buf, int size )
{
    BSenddata_t *p = BsendBuffer.avail;
    char *bufaddr = (char *)buf;

    if (buf == BsendBuffer.buf) {
	/* Special case of buffer at the beginning */
	/* ....; */
	/* Include merge with the next one */
	return;
    }

    /* Otherwise, try to find the location within the buffer */
    while (p) {
	char *p_addr = (char *)p;
	/* Is this at the END of this block */
	if (p_addr + p->size + sizeof(BsendData_t) == bufaddr) {
	    BSendData_t *pnext = p->next;
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
}


#endif
