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
