/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

MPIDI_CH3I_Alloc_mem_list_t *MPIDI_CH3I_Alloc_mem_list_head = NULL;

/*
 * MPIDI_CH3_alloc_mem()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPIDI_CH3_Alloc_mem(size_t size, MPID_Info *info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_Shmem_block_request_result *shm_struct;
    void *ap=NULL;
    MPIDI_CH3I_Alloc_mem_list_t *new_ptr;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ALLOC_MEM);

    shm_struct = (MPIDI_CH3I_Shmem_block_request_result *) 
        MPIU_Malloc(sizeof(MPIDI_CH3I_Shmem_block_request_result));
    /* --BEGIN ERROR HANDLING-- */
    if (shm_struct == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIDI_CH3I_SHM_Get_mem(size, shm_struct);

    if (mpi_errno == MPI_SUCCESS) {
        /* got shared memory. keep track of it by adding it to the list. */

        new_ptr = (MPIDI_CH3I_Alloc_mem_list_t *) 
            MPIU_Malloc(sizeof(MPIDI_CH3I_Alloc_mem_list_t));
        /* --BEGIN ERROR HANDLING-- */
        if (new_ptr == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        new_ptr->shm_struct = shm_struct;

        new_ptr->next = MPIDI_CH3I_Alloc_mem_list_head;
        MPIDI_CH3I_Alloc_mem_list_head = new_ptr;

        ap = shm_struct->addr;
    }

    else {
        /* no shared memory. do a regular malloc and return. */
        ap = MPIU_Malloc(size);

        MPIU_Free(shm_struct);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ALLOC_MEM);
    return ap;
}
