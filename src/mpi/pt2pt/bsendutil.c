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
    int size;
    struct BsendData *next, *prev;
    MPID_Request *request;
    BsendMsg_t msg;
} BsendData_t;

/* BsendBuffer is the structure that describes the overall Bsend buffer */
static struct {
    void        *buf;
    int         size;
    BsendData_t *avail, *next;
} BsendBuffer = { 0, 0, 0, 0 };
    
int MPIR_Bsend_attach( void *buffer, int size )
{
    BsendData_t *p;
    if (BsendBuffer.buf) {
	return MPIR_Err_create_code( MPI_ERR_BUFFER, "**bufexists", 0 );
    }
    if (size < MPI_BSEND_OVERHEAD) {
	return MPIR_Err_create_code( MPI_ERR_OTHER, 
			     "**bsendbufsmall", "**bsendbufsmall %d %d",
			     size, MPI_BSEND_OVERHEAD );
    }

    BsendBuffer.buf = buffer;
    BsendBuffer.size = size;
    BsendBuffer.avail = buffer;
    BsendBuffer.next = 0;
    
    p = (BsendData_t *)buffer;
    p->size = size - sizeof(BsendData_t);
    p->next = p->prev = 0;

    return MPI_SUCCESS;
}

/* This routine must wait until any pending bsends are complete */
int MPIR_Bsend_detach( void *p, int *size )
{
    if (BsendBuffer.next) {
	return MPIR_Err_create_code( MPI_ERR_OTHER, "**notimpl", 0 );
    }
    *size = BsendBuffer.size;
    BsendBuffer.buf = 0;

    return MPI_SUCCESS;
}
