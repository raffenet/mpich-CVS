/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpir_pt2pt.h"

void MPIR_Wait(MPID_Request * request)
{
    while(1)
    {
	MPID_Progress_start();
	
	if ((*request->cc_ptr) != 0)
	{
	    MPID_Progress_wait();
	}
	else
	{
	    MPID_Progress_end();
	    break;
	}
    }
}
