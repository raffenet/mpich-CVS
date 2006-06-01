/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*
 * internal function declarations
 */
MPIG_STATIC void mpig_comm_list_add(MPID_Comm * const comm);
MPIG_STATIC void mpig_comm_list_remove(MPID_Comm * const comm);


/*
 * internal data structures
 */
MPIG_STATIC MPID_Comm * mpig_comm_list = NULL;


/*
 * int mpig_comm_construct([IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator being created
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_construct
int mpig_comm_construct(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_construct);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_construct);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));

    mpi_errno = mpig_topology_comm_construct(comm);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|topology_comm_construct",
	"*globus|topology_comm_construct %C", comm);
    
    mpig_comm_list_add(comm);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, comm->handle, (MPIG_PTR_CAST) comm, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_construct);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_comm_construct() */


/*
 * int mpig_comm_destruct([IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator being destroyed
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_destruct
int mpig_comm_destruct(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_destruct);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_destruct);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));

    /* if this communicator was created by MPIR_Setup_intercomm_localcomm(), then skip the destruction process.  see comments
       below. */
    if (comm->dev.active_list_prev == comm) goto fn_return;
    
    /* a local intracommunicator may be attached to an intercommunicator.  this intracommunicator is created locally (meaning not
       collectively) by MPIR_Setup_intercomm_localcomm() when needed.  the device is not notified of its creation and thus has
       not intialized any of the device information.  we set the active list previous and next fields of the local
       intracommunicator to point at itself so that we can detect when one of these local intracommuncators is being destroyed
       and skip the destruction process. */
    if (comm->comm_kind == MPID_INTERCOMM)
    {
	MPID_Comm * const local_comm = comm->local_comm;
	
	if (local_comm != NULL)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "encountered an intercommunicator with a local intracommunicator: comm="
		MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT ", local_comm"  MPIG_HANDLE_FMT ", local_commp=" MPIG_PTR_FMT, 
		comm->handle, (MPIG_PTR_CAST) comm, local_comm->handle, (MPIG_PTR_CAST) local_comm));
	    
	    local_comm->dev.active_list_prev = local_comm;
	    local_comm->dev.active_list_next = local_comm;
	}
    }

    mpig_comm_list_remove(comm);
    
    mpi_errno = mpig_topology_comm_destruct(comm);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|topology_comm_destruct",
	"*globus|topology_comm_destruct %C", comm);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, comm->handle, (MPIG_PTR_CAST) comm, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_destruct);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_comm_destruct() */


/*
 * void mpig_comm_list_add([IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator to add to the list of active communicators
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_list_add
MPIG_STATIC void mpig_comm_list_add(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_list_add);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_list_add);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));

    comm->dev.user_ref = TRUE;
    comm->dev.active_list_prev = NULL;
    comm->dev.active_list_next = mpig_comm_list;
    if (mpig_comm_list != NULL)
    {
	mpig_comm_list ->dev.active_list_prev = comm;
    }
    mpig_comm_list = comm;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
	"mpig_comm_list=" MPIG_PTR_FMT ", comm->prev=" MPIG_PTR_FMT ", comm->prev->next=" MPIG_PTR_FMT ", comm->next="
	MPIG_PTR_FMT ", comm->next->prev=" MPIG_PTR_FMT, (MPIG_PTR_CAST) mpig_comm_list,
	(MPIG_PTR_CAST) comm->dev.active_list_prev,(MPIG_PTR_CAST)((comm->dev.active_list_prev != NULL) ?
	comm->dev.active_list_prev->dev.active_list_next : NULL), (MPIG_PTR_CAST) comm->dev.active_list_next,
	(MPIG_PTR_CAST)((comm->dev.active_list_next != NULL) ? comm->dev.active_list_next->dev.active_list_prev : NULL)));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
		       "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm))
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_list_add);
    return;
}
/* mpig_comm_list_add() */

/*
 * void mpig_comm_list_remove([IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator to remove from the list of active communicators
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_list_remove
MPIG_STATIC void mpig_comm_list_remove(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_list_remove);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_list_remove);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
	"mpig_comm_list=" MPIG_PTR_FMT ", comm->prev=" MPIG_PTR_FMT ", comm->prev->next=" MPIG_PTR_FMT ", comm->next="
	MPIG_PTR_FMT ", comm->next->prev=" MPIG_PTR_FMT, (MPIG_PTR_CAST) mpig_comm_list,
	(MPIG_PTR_CAST) comm->dev.active_list_prev, (MPIG_PTR_CAST)((comm->dev.active_list_prev != NULL) ?
	comm->dev.active_list_prev->dev.active_list_next : NULL), (MPIG_PTR_CAST) comm->dev.active_list_next,
	(MPIG_PTR_CAST)((comm->dev.active_list_next != NULL) ? comm->dev.active_list_next->dev.active_list_prev : NULL)));

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

    comm->dev.user_ref = FALSE;
    comm->dev.active_list_prev = NULL;
    comm->dev.active_list_next = NULL;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
	"mpig_comm_list=" MPIG_PTR_FMT ", comm->prev=" MPIG_PTR_FMT ", comm->prev->next=" MPIG_PTR_FMT ", comm->next="
	MPIG_PTR_FMT ", comm->next->prev=" MPIG_PTR_FMT, (MPIG_PTR_CAST) mpig_comm_list,
	(MPIG_PTR_CAST) comm->dev.active_list_prev, (MPIG_PTR_CAST)((comm->dev.active_list_prev != NULL) ?
	comm->dev.active_list_prev->dev.active_list_next : NULL), (MPIG_PTR_CAST) comm->dev.active_list_next,
	(MPIG_PTR_CAST)((comm->dev.active_list_next != NULL) ? comm->dev.active_list_next->dev.active_list_prev : NULL)));
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_list_remove);
    return;
}
/* mpig_comm_list_remove */

/*
 * int mpig_comm_list_wait_empty(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_list_wait_empty
int mpig_comm_list_wait_empty(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_list_wait_empty);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_list_wait_empty);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering"));

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
	
	MPID_Progress_start(&pe_state);
	while(mpig_comm_list == comm && comm->ref_count > required_count)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM,
			       "waiting for operations on communicator to complete: comm=" MPIG_HANDLE_FMT ",commp=" MPIG_PTR_FMT,
			       comm->handle, (MPIG_PTR_CAST) comm));
	    mpi_errno = MPID_Progress_wait(&pe_state);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&pe_state);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
	MPID_Progress_end(&pe_state);

	/* MPIR_Comm_release() is never called for builtin communicators, and thus mpig_comm_destruct() is never called, so we
	   call the destruct routine here to release any device related resources still held by the communicator. */
	if (HANDLE_GET_KIND(mpig_comm_list->handle) == HANDLE_KIND_BUILTIN)
	{
	    mpig_comm_destruct(mpig_comm_list);
	}
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_list_wait_empty);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_comm_list_wait_empty() */


/*
 * int mpig_comm_free_hook[IN/MOD] comm)
 *
 * comm [IN/MOD] - communicator being freed
 * mpi_errno [OUT] - MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_free_hook
int mpig_comm_free_hook(MPID_Comm * comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_free_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_free_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, (MPIG_PTR_CAST) comm));

    
    /* clear the user reference flag to indicate that the user no longer holds a reference to this communicator */
    comm->dev.user_ref = FALSE;

    /* call the VMPI CM's hook, allowing it to free the associated vendor communicators */
#   if defined(MPIG_VMPI)
    {
	if mpig_cm_vmpi_dev_comm_free_hook(comm, mpi_errno_p);
	/* FIXME: convert this into a generic CM function table list/array so that any CM can register hooks */
    }
#   endif

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_free_hook);
    return mpi_errno;
}
/* mpig_comm_free_hook() */


#if defined(MPIG_VMPI)

/*
 * int mpig_comm_dup_hook[IN] orig_comm, [IN/MOD] new_comm)
 *
 * orig_comm [IN] - communicator being duplicated
 * new_comm [IN/MOD] - communicator resulting from the duplication
 * mpi_errno [OUT] - MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_comm_dup_hook
int  mpig_comm_dup_hook(MPID_Comm * orig_comm, MPID_Comm * new_comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_comm_dup_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_comm_dup_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: orig_comm=" MPIG_HANDLE_FMT ", orig_commp=" MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT
	", new_commp=" MPIG_PTR_FMT, orig_comm->handle, (MPIG_PTR_CAST) orig_comm, new_comm->handle, (MPIG_PTR_CAST) new_comm));

    /* call the VMPI CM's hook, allowing it to duplication the associated vendor communicators */
    mpig_cm_vmpi_dev_comm_dup_hook(orig_comm, new_comm, mpi_errno_p);
    /* FIXME: convert this into a generic CM function table list/array so that any CM can register hooks */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_comm_dup_hook);
    return mpi_errno;
}
/* mpig_comm_dup_hook() */

#endif /* defined(MPIG_VMPI) */
