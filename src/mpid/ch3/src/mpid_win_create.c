/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_PTHREAD_H
#define THREAD_RETURN_TYPE void *
#elif defined(HAVE_WINTHREADS)
#define THREAD_RETURN_TYPE DWORD
#else
#define THREAD_RETURN_TYPE int
#endif

THREAD_RETURN_TYPE MPIDI_Win_passive_target_thread(void *arg);

volatile int MPIDI_Passive_target_thread_exit_flag=0;

#undef FUNCNAME
#define FUNCNAME MPID_Win_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Win_create(void *base, MPI_Aint size, int disp_unit, MPID_Info *info, 
                    MPID_Comm *comm_ptr, MPID_Win **win_ptr)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_CREATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_WIN_CREATE);
    
    MPIR_Nest_incr();
        
#   if defined(MPIDI_CH3_IMPLEMENTS_WIN_CREATE)
    {
	mpi_errno = MPIDI_CH3_Win_create(base, size, disp_unit, info, comm_ptr, win_ptr);
    }
#   else
    {

#if defined(HAVE_WINTHREADS) && !defined(MPICH_SINGLE_THREADED)
        DWORD dwThreadID;
#endif
        int i, comm_size, rank;
        MPI_Aint *tmp_buf;
        
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
        tmp_buf = (MPI_Aint *) MPIU_Malloc(3*comm_size*sizeof(MPI_Aint));
        /* --BEGIN ERROR HANDLING-- */
        if (!tmp_buf)
        {
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        /* FIXME: This needs to be fixed for heterogeneous systems */
        tmp_buf[3*rank] = MPIU_PtrToAint(base);
        tmp_buf[3*rank+1] = (MPI_Aint) disp_unit;
        tmp_buf[3*rank+2] = (MPI_Aint) (*win_ptr)->handle;
        
        mpi_errno = NMPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                                   tmp_buf, 3 * sizeof(MPI_Aint), MPI_BYTE, 
                                   comm_ptr->handle);   
        
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != MPI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
        
        for (i=0; i<comm_size; i++)
        {
            (*win_ptr)->base_addrs[i] = MPIU_AintToPtr(tmp_buf[3*i]);
            (*win_ptr)->disp_units[i] = (int) tmp_buf[3*i+1];
            (*win_ptr)->all_win_handles[i] = (MPI_Win) tmp_buf[3*i+2];
        }
        
        MPIU_Free(tmp_buf);
        
#ifdef FOOOOOOOOOOOOOOO
        
#ifndef MPICH_SINGLE_THREADED
#ifdef HAVE_PTHREAD_H
        pthread_create(&((*win_ptr)->passive_target_thread_id), NULL,
                       MPIDI_Win_passive_target_thread, (void *) (*win_ptr));  
#elif defined(HAVE_WINTHREADS)
        (*win_ptr)->passive_target_thread_id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MPIDI_Win_passive_target_thread, (*win_ptr), 0, &dwThreadID);
#else
#error Error: No thread package specified.
#endif
#endif

#endif

    }
#   endif

#ifndef MPIDI_CH3_IMPLEMENTS_WIN_CREATE
 fn_exit:
#endif
    MPIR_Nest_decr();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_WIN_CREATE);
    return mpi_errno;
}
