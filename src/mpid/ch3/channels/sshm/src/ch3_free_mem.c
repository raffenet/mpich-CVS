/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Free_mem()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Free_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Free_mem(void *addr)
{
    int mpi_errno = MPI_SUCCESS, found;
    MPIDI_CH3I_Alloc_mem_list_t *prev_ptr, *curr_ptr;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_FREE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_FREE_MEM);

    /* check whether it was allocated as shared memory */

    prev_ptr = curr_ptr = MPIDI_CH3I_Alloc_mem_list_head;
    found = 0;
    while (curr_ptr != NULL) {
        if (addr == curr_ptr->shm_struct->addr) {
            found = 1;

            /* deallocate shared memory */
            mpi_errno = MPIDI_CH3I_SHM_Unlink_and_detach_mem(curr_ptr->shm_struct);
            if (mpi_errno != MPI_SUCCESS) goto fn_exit;
            
            /* remove this entry from the list */

            if (curr_ptr == MPIDI_CH3I_Alloc_mem_list_head)
                MPIDI_CH3I_Alloc_mem_list_head = curr_ptr->next;
            else 
                prev_ptr->next = curr_ptr->next;

            MPIU_Free(curr_ptr->shm_struct);
            MPIU_Free(curr_ptr);

            break;
        }
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    if (!found) /* it wasn't shared memory. call MPIU_Free. */
        MPIU_Free(addr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_FREE_MEM);
    return mpi_errno;
}
