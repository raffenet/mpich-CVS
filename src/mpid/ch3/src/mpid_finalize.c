/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"


#undef FUNCNAME
#define FUNCNAME MPID_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Finalize()
{
    MPID_Progress_state progress_state;
    int mpi_errno = MPI_SUCCESS;
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_FINALIZE);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    /*
     * Wait for all posted receives to complete.  For now we are not doing this since it will cause invalid programs to hang.
     * The side effect of not waiting is that posted any source receives may erroneous blow up.
     *
     * For now, we are placing a warning at the end of MPID_Finalize() to inform the user if any outstanding posted receives
     * exist.
     */
    
    /* FIXME: insert while loop here to wait for outstanding posted receives to complete */


    MPID_VCRT_Release(MPIR_Process.comm_self->vcrt);
    MPID_VCRT_Release(MPIR_Process.comm_world->vcrt);
		
    /*
     * Initiate close protocol for all active VCs
     */
    for (;;)
    {
	int i;
	MPIDI_PG_t * pg;
	MPIDI_VC_t * vc;

	MPIDI_PG_Get_next(&pg);
	if (pg == NULL)
	{
	    break;
	}

	for (i = 0; i < MPIDI_PG_Get_size(pg); i++)
	{
	    /* If the VC is myself then skip the close message */
	    if (pg == MPIDI_Process.my_pg && i == MPIDI_Process.my_pg_rank)
	    {
		continue;
	    }

	    MPIDI_PG_Get_vcr(pg, i, &vc);
	    if (vc->state == MPIDI_VC_STATE_ACTIVE || vc->state == MPIDI_VC_STATE_REMOTE_CLOSE)
	    {
		MPIDI_CH3_Pkt_t upkt;
		MPIDI_CH3_Pkt_close_t * close_pkt = &upkt.close;
		MPID_Request * sreq;
		    
		close_pkt->type = MPIDI_CH3_PKT_CLOSE;
		close_pkt->ack = (vc->state == MPIDI_VC_STATE_ACTIVE) ? FALSE : TRUE;
		
		mpi_errno = MPIDI_CH3_iStartMsg(vc, close_pkt, sizeof(*close_pkt), &sreq);
		/* --BEGIN ERROR HANDLING-- */
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|send_close_ack", 0);
		    continue;
		}
		/* --END ERROR HANDLING-- */
		    
		if (sreq != NULL)
		{
		    MPID_Request_release(sreq);
		}

		/* MT: this is not thread safe */
		MPIDI_Outstanding_close_ops += 1;
		MPIDI_DBG_PRINTF((30, FCNAME, "sending close(%s) to %d, ops = %d", close_pkt->ack ? "TRUE" : "FALSE",
				       i, MPIDI_Outstanding_close_ops));
		    
		if (vc->state == MPIDI_VC_STATE_ACTIVE)
		{ 
		    vc->state = MPIDI_VC_STATE_LOCAL_CLOSE;
		}
		else /* if (vc->state == MPIDI_VC_STATE_REMOTE_CLOSE) */
		{
		    vc->state = MPIDI_VC_STATE_CLOSE_ACKED;
		}
	    }
	    else
	    {
		MPIDI_DBG_PRINTF((30, FCNAME, "not sending a close to %d, vc in state %s", i,
				  MPIDI_VC_Get_state_description(vc->state)));
	    }
	}
    }

    /*
     * Wait for all VCs to finish the close protocol
     */
    MPID_Progress_start(&progress_state);
    while(MPIDI_Outstanding_close_ops > 0)
    {
	MPID_Progress_wait(&progress_state);
    }
    MPID_Progress_end(&progress_state);


    if (MPIDI_Process.warnings_enabled)
    {
	if (MPIDI_Process.recvq_posted_head != NULL)
	{
	    /* XXX - insert code to print posted receive queue */
	    MPIU_Msg_printf("Warning: program exiting with outstanding receive requests\n");
	}
    }


    mpi_errno = MPIDI_CH3_Finalize();
    
    MPIDI_Process.my_pg = NULL;
    
    MPIU_Free(MPIDI_Process.processor_name);

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_FINALIZE);
    return mpi_errno;
}
