/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


MPIG_STATIC MPID_Comm * mpig_comm_list = NULL;


/*
 * void mpig_comm_list_add([IN] comm)
 *
 * comm [IN] - communicator to add to the list of active communicators
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_list_add
void mpig_comm_list_add(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_list_add);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_list_add);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm))

    comm->dev.user_ref = TRUE;
    comm->dev.active_list_prev = NULL;
    comm->dev.active_list_next = mpig_comm_list;
    if (mpig_comm_list != NULL)
    {
	mpig_comm_list ->dev.active_list_prev = comm;
    }
    mpig_comm_list = comm;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
		       ", mpig_comm_list=" MPIG_PTR_FMT ", comm->prev=" MPIG_PTR_FMT ", comm->prev->next=" MPIG_PTR_FMT
		       ", comm->next=" MPIG_PTR_FMT ", comm->next->prev=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) mpig_comm_list,
		       (MPIG_PTR_CAST) comm->dev.active_list_prev,
		       (MPIG_PTR_CAST)((comm->dev.active_list_prev != NULL) ?
				       comm->dev.active_list_prev->dev.active_list_next : NULL),
		       (MPIG_PTR_CAST) comm->dev.active_list_next,
		       (MPIG_PTR_CAST)((comm->dev.active_list_next != NULL) ?
				       comm->dev.active_list_next->dev.active_list_prev : NULL)));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm))
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_list_add);
    return;
}


/*
 * void mpig_comm_list_remove([IN] comm)
 *
 * comm [IN] - communicator to remove from the list of active communicators
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_list_remove
void mpig_comm_list_remove(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_list_remove);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_list_remove);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm))
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
		       ", mpig_comm_list=" MPIG_PTR_FMT ", comm->prev=" MPIG_PTR_FMT ", comm->prev->next=" MPIG_PTR_FMT
		       ", comm->next=" MPIG_PTR_FMT ", comm->next->prev=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) mpig_comm_list,
		       (MPIG_PTR_CAST) comm->dev.active_list_prev,
		       (MPIG_PTR_CAST)((comm->dev.active_list_prev != NULL) ?
				       comm->dev.active_list_prev->dev.active_list_next : NULL),
		       (MPIG_PTR_CAST) comm->dev.active_list_next,
		       (MPIG_PTR_CAST)((comm->dev.active_list_next != NULL) ?
				       comm->dev.active_list_next->dev.active_list_prev : NULL)));

    MPIU_Assert(HANDLE_GET_KIND(comm->handle) == HANDLE_KIND_BUILTIN || comm->dev.user_ref == FALSE);
    
    if (comm->dev.active_list_prev == NULL)
    {
	mpig_comm_list = comm->dev.active_list_next;
    }
    else
    {
	comm->dev.active_list_prev->dev.active_list_next = comm->dev.active_list_next;
    }
    
    if (comm->dev.active_list_next != NULL)
    {
	comm->dev.active_list_next->dev.active_list_prev = comm->dev.active_list_prev;
    }

    comm->dev.active_list_prev = NULL;
    comm->dev.active_list_next = NULL;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
		       ", mpig_comm_list=" MPIG_PTR_FMT ", comm->prev=" MPIG_PTR_FMT ", comm->prev->next=" MPIG_PTR_FMT
		       ", comm->next=" MPIG_PTR_FMT ", comm->next->prev=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) mpig_comm_list,
		       (MPIG_PTR_CAST) comm->dev.active_list_prev,
		       (MPIG_PTR_CAST)((comm->dev.active_list_prev != NULL) ?
				       comm->dev.active_list_prev->dev.active_list_next : NULL),
		       (MPIG_PTR_CAST) comm->dev.active_list_next,
		       (MPIG_PTR_CAST)((comm->dev.active_list_next != NULL) ?
				       comm->dev.active_list_next->dev.active_list_prev : NULL)));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm))
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_list_remove);
    return;
}


/*
 * void mpig_comm_list_wait_empty([IN/OUT] mpi_errno, [OUT] failed)
 *
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_list_wait_empty
void mpig_comm_list_wait_empty(int * const mpi_errno_p, bool_t * const failed_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_list_wait_empty);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_list_wait_empty);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "entering: mpi_errno=0x%08x", *mpi_errno_p));

    *failed_p = FALSE;
    
    /* release application references to communicators */
    {
	MPID_Comm * comm = mpig_comm_list;
	while (comm != NULL)
	{
	    MPID_Comm * const comm_next = comm->dev.active_list_next;

	    MPIU_Assert(HANDLE_GET_KIND(comm->handle) != HANDLE_KIND_INVALID);
	
	    if (comm->dev.user_ref && HANDLE_GET_KIND(comm->handle) != HANDLE_KIND_BUILTIN)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
				   "releasing communicator not freed by application: comm=" MPIG_HANDLE_FMT ",commp="
				   MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));
		comm->dev.user_ref = FALSE;
		MPIR_Comm_release(comm);
	    }

	    comm = comm_next;
	}
    }
    
    /* wait for all requests on all communicators to complete */
    while (mpig_comm_list != NULL)
    {
	const MPID_Comm * comm = mpig_comm_list;
	const int required_count = (HANDLE_GET_KIND(mpig_comm_list->handle) == HANDLE_KIND_BUILTIN) ? 1 : 0;
	MPID_Progress_state pe_state;
	int mpi_errno;
	
	MPID_Progress_start(&pe_state);
	while(mpig_comm_list == comm && comm->ref_count > required_count)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
			       "waiting for operations on communicator to complete: comm=" MPIG_HANDLE_FMT ",commp=" MPIG_PTR_FMT,
			       comm->handle, (MPIG_PTR_CAST) comm));
	    mpi_errno = MPID_Progress_wait(&pe_state);
	    if (mpi_errno)
	    {
		/* --BEGIN ERROR HANDLING-- */
		*mpi_errno_p = mpi_errno;
		MPID_Progress_end(&pe_state);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
	MPID_Progress_end(&pe_state);

	if (HANDLE_GET_KIND(mpig_comm_list->handle) == HANDLE_KIND_BUILTIN)
	{
	    mpig_comm_list_remove(mpig_comm_list);
	}
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "exiting: mpi_errno=0x%08x, failed=%s",
		       *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_list_wait_empty);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	*failed_p = TRUE;
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_comm_list_wait_empty() */
