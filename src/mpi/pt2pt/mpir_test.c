/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

int MPIR_Test(MPID_Request * request)
{
    int req_complete;
    
    /* NOTE: We do not use MPID_Progress_start() and MPID_Progress_end() in
       this function since those routines are only necessary when calling
       MPIR_Progress_wait(). */
    req_complete = (*request->cc_ptr == 0);
    if (!req_complete)
    {
	MPID_Progress_test();
	req_complete = (*request->cc_ptr == 0);
    }

    return req_complete;
}
