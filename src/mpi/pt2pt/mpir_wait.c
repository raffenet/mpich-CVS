/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

void MPIR_Wait(MPID_Request * request)
{
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_WAIT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_WAIT);
    MPIU_DBG_PRINTF((" MPIR_Wait: entering\n"));
    while((*request->cc_ptr) != 0)
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
    MPIU_DBG_PRINTF((" MPIR_Wait: exiting\n"));
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_WAIT);
}
