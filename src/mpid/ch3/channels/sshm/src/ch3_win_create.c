/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Win_create()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS, i, comm_size, rank, found, result;
    void **tmp_buf, *offset=0;
    MPIDI_CH3I_Alloc_mem_list_t *curr_ptr;
        
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_WIN_CREATE);
    MPIDI_STATE_DECL(MPID_STATE_MEMCPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_WIN_CREATE);

    /* The first part of the code below is the same as in MPID_Win_create, until the point 
       where all processes check whether win_base is in shared memory. */

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    
    *win_ptr = (MPID_Win *)MPIU_Handle_obj_alloc( &MPID_Win_mem );
    /* --BEGIN ERROR HANDLING-- */
    if (!(*win_ptr))
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    (*win_ptr)->fence_cnt = 0;
    (*win_ptr)->base = base;
    (*win_ptr)->size = size;
    (*win_ptr)->disp_unit = disp_unit;
    (*win_ptr)->start_group_ptr = NULL; 
    (*win_ptr)->start_assert = 0; 
    (*win_ptr)->attributes = NULL;
    (*win_ptr)->rma_ops_list = NULL;
    (*win_ptr)->lock_granted = 0;
    (*win_ptr)->current_lock_type = MPID_LOCK_NONE;
    (*win_ptr)->shared_lock_ref_cnt = 0;
    (*win_ptr)->lock_queue = NULL;
    (*win_ptr)->my_counter = 0;
    (*win_ptr)->my_pt_rma_puts_accs = 0;
    
    
    MPIR_Nest_incr();
        
    mpi_errno = NMPI_Comm_dup(comm_ptr->handle, &((*win_ptr)->comm));
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    /* allocate memory for the base addresses, disp_units, and
       completion counters of all processes */ 
    (*win_ptr)->base_addrs = (void **) MPIU_Malloc(comm_size *
                                                   sizeof(void *));   
    /* --BEGIN ERROR HANDLING-- */
    if (!(*win_ptr)->base_addrs)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    (*win_ptr)->disp_units = (int *) MPIU_Malloc(comm_size * sizeof(int));
    /* --BEGIN ERROR HANDLING-- */
    if (!(*win_ptr)->disp_units)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    (*win_ptr)->all_win_handles = (MPI_Win *) MPIU_Malloc(comm_size * sizeof(MPI_Win));
    /* --BEGIN ERROR HANDLING-- */
    if (!(*win_ptr)->all_win_handles)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    (*win_ptr)->pt_rma_puts_accs = (int *) MPIU_Calloc(comm_size, sizeof(int));
    /* --BEGIN ERROR HANDLING-- */
    if (!(*win_ptr)->pt_rma_puts_accs)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    /* get the addresses of the windows, window objects, and completion counters
       of all processes */  
    
    /* allocate temp. buffer for communication */
    tmp_buf = (void **) MPIU_Malloc(3*comm_size*sizeof(void*));
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_buf)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    /* FIXME: This needs to be fixed for heterogeneous systems */
    tmp_buf[3*rank] = base;
    tmp_buf[3*rank+1] = MPIU_IntToPtr(disp_unit);
    tmp_buf[3*rank+2] = MPIU_IntToPtr((*win_ptr)->handle);
    
    mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                               tmp_buf, 3 * sizeof(void *), MPI_BYTE, 
                               comm_ptr->handle);   
    
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    MPIR_Nest_decr();
    
    for (i=0; i<comm_size; i++)
    {
        (*win_ptr)->base_addrs[i] = tmp_buf[3*i];
        (*win_ptr)->disp_units[i] = MPIU_PtrToInt(tmp_buf[3*i+1]);
        (*win_ptr)->all_win_handles[i] = MPIU_PtrToInt(tmp_buf[3*i+2]);
    }
    
    MPIU_Free(tmp_buf);


    /* The code below is different from the generic MPID_Win_create */

    /* All processes first check whether their base address refers to an address in 
       shared memory. If everyone's address is in shared memory, we set 
       MPIDI_Use_optimized_rma=1 to indicate that shared memory optimizations are possible.
       If even one process's win_base is not in shared memory, we revert to the generic 
       implementation of RMA in CH3 by setting MPIDI_Use_optimized_rma=0. */

    curr_ptr = MPIDI_CH3I_Alloc_mem_list_head;
    found = 0;
    while (curr_ptr != NULL) {
        if ((curr_ptr->shm_struct->addr <= base) && 
            (base < (void *) ((char *) curr_ptr->shm_struct->addr + curr_ptr->shm_struct->size))) {
            found = 1;
            offset = (void *) ((char *) curr_ptr->shm_struct->addr - (char *) base);
            break;
        }
        curr_ptr = curr_ptr->next;
    }

    mpi_errno = NMPI_Allreduce(&found, &result, 1, MPI_INT, MPI_BAND, comm_ptr->handle);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    if (result == 0) { /* not all in shared memory, can't be optimized */
        MPIDI_Use_optimized_rma = 0;
        (*win_ptr)->shm_structs = NULL;
    }
    else {   /* all in shared memory. can be optimized */
        MPIDI_Use_optimized_rma = 1;

        /* allocate memory for the shm_structs */
        (*win_ptr)->shm_structs = (MPIDI_CH3I_Shmem_block_request_result *) 
            MPIU_Malloc(comm_size * sizeof(MPIDI_CH3I_Shmem_block_request_result));

        /* --BEGIN ERROR HANDLING-- */
        if ((*win_ptr)->shm_structs == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        /* allocate memory for the offsets from base of shared memory */
        (*win_ptr)->offsets = (void **) MPIU_Malloc(comm_size * sizeof(void *));
        /* --BEGIN ERROR HANDLING-- */
        if ((*win_ptr)->offsets == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        /* copy this process's shmem struct into right location in array of shmem structs */
	MPIDI_FUNC_ENTER(MPID_STATE_MEMCPY);
        memcpy(&((*win_ptr)->shm_structs[rank]), curr_ptr->shm_struct, 
               sizeof(MPIDI_CH3I_Shmem_block_request_result));
	MPIDI_FUNC_EXIT(MPID_STATE_MEMCPY);

        /* copy this process's offset into right location in array of offsets */
        (*win_ptr)->offsets[rank] = offset;

        /* collect everyone's shm_structs and offsets */

        MPIR_Nest_incr();
        mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   (*win_ptr)->shm_structs, 
                                   sizeof(MPIDI_CH3I_Shmem_block_request_result), 
                                   MPI_BYTE, comm_ptr->handle);   
        
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   (*win_ptr)->offsets, sizeof(void *), 
                                   MPI_BYTE, comm_ptr->handle);   

        MPIR_Nest_decr();
        
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        /* each process now attaches to the shared memory segments of other processes, so that 
           direct RMA is possible. We use the addr field in the shmem struct to store the 
           newly mapped addresses. */

        for (i=0; i<comm_size; i++) {
            if (i != rank) {
                mpi_errno = MPIDI_CH3I_SHM_Attach_notunlink_mem( &((*win_ptr)->shm_structs[i]) );

                /* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                    goto fn_exit;
                }
                /* --END ERROR HANDLING-- */
            }
        }
    }

    (*win_ptr)->epoch_grp_ptr = NULL;
    (*win_ptr)->epoch_grp_ranks_in_win = NULL;
        
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_WIN_CREATE);
    return mpi_errno;
}
