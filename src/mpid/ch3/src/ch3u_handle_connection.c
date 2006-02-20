/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* Count the number of outstanding close requests */
static volatile int MPIDI_Outstanding_close_ops = 0;

/* FIXME: What is this routine for?
   It appears to be used only in ch3_progress, ch3_progress_connect, or
   ch3_progress_sock files.  Is this a general operation, or does it 
   belong in util/sock ? It appears to be used in multiple channels, 
   but probably belongs in mpid_vc, along with the vc exit code that 
   is currently in MPID_Finalize */

/* FIXME: The only event is event_terminated.  Should this have 
   a different name/expected function? */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPIDI_CH3U_Handle_connection - handle connection event

  Input Parameters:
+ vc - virtual connection
. event - connection event

  NOTE:
  At present this function is only used for connection termination
@*/
int MPIDI_CH3U_Handle_connection(MPIDI_VC_t * vc, MPIDI_VC_Event_t event)
{
    int inuse;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_CONNECTION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_CONNECTION);

    switch (event)
    {
	case MPIDI_VC_EVENT_TERMINATED:
	{
	    switch (vc->state)
	    {
		case MPIDI_VC_STATE_CLOSE_ACKED:
		{
		    MPIU_DBG_PrintVCState2(vc, MPIDI_VC_STATE_INACTIVE);
		    MPIU_DBG_MSG(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_INACTIVE");
		    vc->state = MPIDI_VC_STATE_INACTIVE;
		    /* FIXME: Decrement the reference count?  Who increments? */
		    /* FIXME: The reference count is often already 0.  But
		       not always */
		    /* MPIU_Object_set_ref(vc, 0); ??? */

		    /*
		     * FIXME: The VC used in connect accept has a NULL process group
		     */
		    if (vc->pg != NULL && vc->ref_count == 0)
		    { 
			/* FIXME: Who increments the reference count that
			   this is decrementing? */
			/* When the reference count for a vc becomes zero, 
			   decrement the reference count
			   of the associated process group.  */
			/* FIXME: This should be done when the reference 
			   count of the vc is first decremented */
			MPIDI_PG_Release_ref(vc->pg, &inuse);
			if (inuse == 0) {
			    MPIDI_PG_Destroy(vc->pg);
			}
		    }

		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops -= 1;
		    MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,
             "outstanding close operations = %d", MPIDI_Outstanding_close_ops);
	    
		    if (MPIDI_Outstanding_close_ops == 0)
		    {
			MPIDI_CH3_Progress_signal_completion();
		    }

		    break;
		}

		default:
		{
		    MPIU_DBG_MSG_D(CH3_CONNECT,TYPICAL,"Unhandled connection state %d when closing connection",vc->state);
		    mpi_errno = MPIR_Err_create_code(
			MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|unhandled_connection_state",
			"**ch3|unhandled_connection_state %p %d", vc, event);
		    break;
		}
	    }

	    break;
	}
    
	default:
	{
	    break;
	}
    }
	
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_CONNECTION);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_VC_SendClose
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPIDI_CH3U_VC_SendClose - Initiate a close on a virtual connection
  
  Input Parameters:
+ vc - Virtual connection to close
- i  - rank of virtual connection within a process group (used for debugging)

  Notes:
  The current state of this connection must be either 'MPIDI_VC_STATE_ACTIVE' 
  or 'MPIDI_VC_STATE_REMOTE_CLOSE'.  
  @*/
int MPIDI_CH3U_VC_SendClose( MPIDI_VC_t *vc, int rank )
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_close_t * close_pkt = &upkt.close;
    MPID_Request * sreq;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);

    MPIU_Assert( vc->state == MPIDI_VC_STATE_ACTIVE || 
		 vc->state == MPIDI_VC_STATE_REMOTE_CLOSE 
#ifdef MPIDI_CH3_USES_SSHM
		 /* FIXME: Remove this IFDEF */
		 /* sshm queues are uni-directional.  A VC that is connected 
		 * in the read direction is marked MPIDI_VC_STATE_INACTIVE
		 * so that a connection will be formed on the first write.  
		 * Since the other side is marked MPIDI_VC_STATE_ACTIVE for 
		 * writing 
		 * we need to initiate the close protocol on the read side 
		 * even if the write state is MPIDI_VC_STATE_INACTIVE. */
		|| ((vc->state == MPIDI_VC_STATE_INACTIVE) && vc->ch.shm_read_connected)
#endif
		 );

    MPIDI_Pkt_init(close_pkt, MPIDI_CH3_PKT_CLOSE);
    close_pkt->ack = (vc->state == MPIDI_VC_STATE_ACTIVE) ? FALSE : TRUE;
    
    /* MT: this is not thread safe */
    MPIDI_Outstanding_close_ops += 1;
    MPIU_DBG_MSG_FMT(CH3_CONNECT,VERBOSE,(MPIU_DBG_FDEST,
				  "sending close(%s) on vc (pg=%p) %p to rank %d, ops = %d", 
				  close_pkt->ack ? "TRUE" : "FALSE", vc->pg, vc, 
				  rank, MPIDI_Outstanding_close_ops));
		    

    /*
     * A close packet acknowledging this close request could be
     * received during iStartMsg, therefore the state must
     * be changed before the close packet is sent.
     */
    if (vc->state == MPIDI_VC_STATE_ACTIVE) {
	MPIU_DBG_PrintVCState2(vc, MPIDI_VC_STATE_LOCAL_CLOSE);
	MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_LOCAL_CLOSE (%p)", vc);
	vc->state = MPIDI_VC_STATE_LOCAL_CLOSE;
    }
    else {
	MPIU_Assert( vc->state == MPIDI_VC_STATE_REMOTE_CLOSE );
	MPIU_DBG_PrintVCState2(vc, MPIDI_VC_STATE_CLOSE_ACKED);
	MPIU_DBG_MSG_P(CH3_CONNECT,TYPICAL,"Setting state to VC_STATE_CLOSE_ACKED (%p)",vc);
	vc->state = MPIDI_VC_STATE_CLOSE_ACKED;
    }
		
    mpi_errno = MPIDI_CH3_iStartMsg(vc, close_pkt, sizeof(*close_pkt), &sreq);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,
		     "**ch3|send_close_ack");
    }
    
    if (sreq != NULL) {
	MPID_Request_release(sreq);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_VC_SENDCLOSE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_VC_WaitForClose
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
  MPIDI_CH3U_VC_WaitForClose - Wait for all virtual connections to close
  @*/
int MPIDI_CH3U_VC_WaitForClose( void )
{
    MPID_Progress_state progress_state;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);

    MPID_Progress_start(&progress_state);
    while(MPIDI_Outstanding_close_ops > 0) {
	MPIU_DBG_MSG_D(CH3_CONNECT,VERBOSE,"Waiting for %d close operations",
		       MPIDI_Outstanding_close_ops);
	mpi_errno = MPID_Progress_wait(&progress_state);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS) {
	    MPID_Progress_end(&progress_state);
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER,"**ch3|close_progress");
	    break;
	}
	/* --END ERROR HANDLING-- */
    }
    MPID_Progress_end(&progress_state);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_VC_WAITFORCLOSE);
    return mpi_errno;
}
