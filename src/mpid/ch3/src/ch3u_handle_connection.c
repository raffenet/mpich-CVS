/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

volatile int MPIDI_Outstanding_close_ops = 0;


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_connection
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_connection(MPIDI_VC_t * vc, MPIDI_VC_Event_t event)
{
    int inuse;
    int mpi_errno = MPI_SUCCESS;


    switch (event)
    {
	case MPIDI_VC_EVENT_TERMINATED:
	{
	    switch (vc->state)
	    {
		case MPIDI_VC_STATE_CLOSE_ACKED:
		{
		    vc->state = MPIDI_VC_STATE_INACTIVE;
		    /* MPIU_Object_set_ref(vc, 0); ??? */

		    /*
		     * FIXME: The VC used in connect accept has a NULL process group
		     */
		    if (vc->pg != NULL)
		    { 
			MPIDI_PG_Release_ref(vc->pg, &inuse);
			if (inuse == 0)
			{
			    MPIDI_PG_Destroy(vc->pg);
			}
		    }

		    /* MT: this is not thread safe */
		    MPIDI_Outstanding_close_ops -= 1;
		    /*printf("[%d] handle ops = %d\n", MPIDI_Process.my_pg_rank, MPIDI_Outstanding_close_ops);fflush(stdout);*/
	    
		    if (MPIDI_Outstanding_close_ops == 0)
		    {
			MPIDI_CH3_Progress_signal_completion();
		    }

		    break;
		}

		default:
		{
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
	
    return mpi_errno;
}
