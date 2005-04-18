/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#if (USE_THREAD_IMPL == MPICH_THREAD_IMPL_NOT_IMPLEMENTED)
#define MPIDI_Acceptq_lock() MPID_Thread_lock(&MPIDI_CH3I_Process.acceptq_mutex)
#define MPIDI_Acceptq_unlock() MPID_Thread_unlock(&MPIDI_CH3I_Process.acceptq_mutex)
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
    MPIDI_CH3I_Acceptq_t *q_item;
    int mpi_errno=MPI_SUCCESS;

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
    q_item->next = NULL;

    MPIDI_Acceptq_lock();

    if  (MPIDI_CH3I_Process.acceptq_tail != NULL)
        MPIDI_CH3I_Process.acceptq_tail->next = q_item;
    else
        MPIDI_CH3I_Process.acceptq_head = q_item;
    
    MPIDI_CH3I_Process.acceptq_tail = q_item;

    MPIDI_Acceptq_unlock();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_ENQUEUE);
    return mpi_errno;
}


/* Attempt to dequeue a vc from the accept queue. If the queue is
   empty, return a NULL vc. */
int MPIDI_CH3I_Acceptq_dequeue(MPIDI_VC_t ** vc)
{
    MPIDI_CH3I_Acceptq_t *q_item;
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);

    MPIDI_Acceptq_lock();
    if (MPIDI_CH3I_Process.acceptq_head != NULL) {
        q_item = MPIDI_CH3I_Process.acceptq_head;
        MPIDI_CH3I_Process.acceptq_head = q_item->next;

        if (MPIDI_CH3I_Process.acceptq_head == NULL) 
            MPIDI_CH3I_Process.acceptq_tail = NULL;

        *vc = q_item->vc;
        MPIU_Free(q_item);
    }
    else
        *vc = NULL;

    MPIDI_Acceptq_unlock();

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ACCEPTQ_DEQUEUE);
    return mpi_errno;
}
