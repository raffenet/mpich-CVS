/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2001 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 8
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem = { 0, 0, 0, 0, 0,
					 sizeof(MPID_Request), MPID_Request_direct,
					 MPID_REQUEST_PREALLOC, };

MPID_Request * mm_request_alloc()
{
    MPID_Request *p;
    p = MPIU_Handle_obj_alloc(&MPID_Request_mem);
    if (p == NULL)
	return p;
    p->cc_ptr = &p->cc;
    p->mm.rcar[0].freeme = FALSE;
    p->mm.rcar[1].freeme = FALSE;
    p->mm.wcar[0].freeme = FALSE;
    p->mm.wcar[1].freeme = FALSE;
    p->mm.next_ptr = NULL;
    return p;
}

void mm_request_free(MPID_Request *request_ptr)
{
    MPIU_Handle_obj_free(&MPID_Request_mem, request_ptr);
}

void MPID_Request_free(MPID_Request *request_ptr)
{
    mm_request_free(request_ptr);
}
