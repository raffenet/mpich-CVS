/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
#  ifdef MPIDI_CH3_USES_ACCEPTQ
#   define MPIDI_Acceptq_lock() MPID_Thread_lock(&MPIDI_CH3I_Process.acceptq_mutex)
#   define MPIDI_Acceptq_unlock() MPID_Thread_unlock(&MPIDI_CH3I_Process.acceptq_mutex)
#  endif
#else
#define MPIDI_Acceptq_lock()
#define MPIDI_Acceptq_unlock()
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Acceptq_enqueue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Acceptq_enqueue(MPIDI_VC_t * vc)
{
    int mpi_errno=MPI_SUCCESS;
#ifdef MPIDI_CH3_USES_ACCEPTQ
    MPIDI_CH3I_Acceptq_t *q_item;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);

    q_item = (MPIDI_CH3I_Acceptq_t *)
        MPIU_Malloc(sizeof(MPIDI_CH3I_Acceptq_t)); 
    /* --BEGIN ERROR HANDLING-- */
    if (q_item == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    q_item->vc = vc;

    MPIDI_Acceptq_lock();

    q_item->next = MPIDI_CH3I_Process.acceptq_head;
    MPIDI_CH3I_Process.acceptq_head = q_item;
    
    MPIDI_Acceptq_unlock();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);
#endif
    return mpi_errno;
}


/* Attempt to dequeue a vc from the accept queue. If the queue is
   empty or the port_name_tag doesn't match, return a NULL vc. */
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_VC_t ** vc, int port_name_tag)
{
    int mpi_errno=MPI_SUCCESS;
#ifdef MPIDI_CH3_USES_ACCEPTQ
    MPIDI_CH3I_Acceptq_t *q_item, *prev;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    MPIDI_Acceptq_lock();

    *vc = NULL;
    q_item = MPIDI_CH3I_Process.acceptq_head;
    prev = q_item;
    while (q_item != NULL)
    {
	if (q_item->vc->ch.port_name_tag == port_name_tag)
	{
	    *vc = q_item->vc;

	    if ( q_item == MPIDI_CH3I_Process.acceptq_head )
		MPIDI_CH3I_Process.acceptq_head = q_item->next;
	    else
		prev->next = q_item->next;

	    MPIU_Free(q_item);
	    break;;
	}
	else
	{
	    prev = q_item;
	    q_item = q_item->next;
	}
    }

    MPIDI_Acceptq_unlock();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);
#endif
    return mpi_errno;
}
