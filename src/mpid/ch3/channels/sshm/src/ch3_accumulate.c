/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Accumulate()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *target_addr, *tmp_buf;
    MPI_Aint true_lb, true_extent, extent;
    MPI_User_function *uop;
    MPIDU_Process_lock_t *locks_base_addr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ACCUMULATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ACCUMULATE);

    target_addr = (char *) win_ptr->shm_structs[target_rank].addr + 
                  (long) win_ptr->offsets[target_rank] +
                  win_ptr->disp_units[target_rank] * target_disp;

    locks_base_addr = win_ptr->locks->addr;

    if (op == MPI_REPLACE) {
        /* simply do a memcpy */

        /* all accumulate operations need to be done atomically. If the process does 
           not already have an exclusive lock, acquire it */

        if (win_ptr->pt_rma_excl_lock == 0)
            MPIDU_Process_lock( &locks_base_addr[target_rank] );

        mpi_errno = MPIR_Localcopy (origin_addr, origin_count, origin_datatype, 
                                    target_addr, target_count, target_datatype);

        if (win_ptr->pt_rma_excl_lock == 0)
            MPIDU_Process_unlock( &locks_base_addr[target_rank] );

        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */
    }

    else {

        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
        {
            /* get the function by indexing into the op table */
            uop = MPIR_Op_table[op%16 - 1];
        }
        else
        {
            /* --BEGIN ERROR HANDLING-- */
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opnotpredefined", "**opnotpredefined %d", op );
            goto fn_exit;
            /* --END ERROR HANDLING-- */
        }
    
        if ((HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) &&
            (HANDLE_GET_KIND(origin_datatype) == HANDLE_KIND_BUILTIN)) {
                /* basic datatype on origin and target */
            if (win_ptr->pt_rma_excl_lock == 0)
                MPIDU_Process_lock( &locks_base_addr[target_rank] );

            (*uop)(origin_addr, target_addr, &origin_count, &origin_datatype);

            if (win_ptr->pt_rma_excl_lock == 0)
                MPIDU_Process_unlock( &locks_base_addr[target_rank] );
        }
        else {

            /* allocate a tmp buffer of extent equal to extent of target buf */

            MPIR_Nest_incr();
            mpi_errno = NMPI_Type_get_true_extent(target_datatype, 
                                                  &true_lb, &true_extent);
            MPIR_Nest_decr();
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */

            MPID_Datatype_get_extent_macro(target_datatype, extent); 

            tmp_buf = MPIU_Malloc(target_count * (MPIR_MAX(extent,true_extent)));  
            /* --BEGIN ERROR HANDLING-- */
            if (!tmp_buf)
            {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                                  "**nomem", 0 );
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */

            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);


            /*  copy origin buffer into tmp_buf with same datatype as target. */

            mpi_errno = MPIR_Localcopy (origin_addr, origin_count, origin_datatype, 
                                        tmp_buf, target_count, target_datatype);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno)
            {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
                goto fn_exit;
            }
            /* --END ERROR HANDLING-- */
            
            if (HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN) {
                /* basic datatype on target. call uop. */
                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_lock( &locks_base_addr[target_rank] );

                (*uop)(tmp_buf, target_addr, &target_count, &target_datatype);

                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_unlock( &locks_base_addr[target_rank] );
            }
            else
            {
                /* derived datatype on target */
                MPID_Segment *segp;
                DLOOP_VECTOR *dloop_vec;
                MPI_Aint first, last;
                int vec_len, i, type_size, count;
                MPI_Datatype type;
                MPID_Datatype *dtp;
                
                segp = MPID_Segment_alloc();
                /* --BEGIN ERROR HANDLING-- */
                if (!segp)
                {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
                    return mpi_errno;
                }
                /* --END ERROR HANDLING-- */

                MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
                first = 0;
                last  = SEGMENT_IGNORE_LAST;
                
                MPID_Datatype_get_ptr(target_datatype, dtp);
                vec_len = dtp->n_contig_blocks * target_count + 1; 
                /* +1 needed because Rob says so */

                dloop_vec = (DLOOP_VECTOR *)
                    MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
                /* --BEGIN ERROR HANDLING-- */
                if (!dloop_vec)
                {
                    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
                    return mpi_errno;
                }
                /* --END ERROR HANDLING-- */
                
                MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);
                
                type = dtp->eltype;
                type_size = MPID_Datatype_get_basic_size(type);

                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_lock( &locks_base_addr[target_rank] );
                for (i=0; i<vec_len; i++)
                {
                    count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
                    (*uop)((char *)tmp_buf + MPIU_PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                           (char *)target_addr + MPIU_PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                           &count, &type);
                }
                if (win_ptr->pt_rma_excl_lock == 0)
                    MPIDU_Process_unlock( &locks_base_addr[target_rank] );
                
                MPID_Segment_free(segp);
                MPIU_Free(dloop_vec);
            }
        
            /* free the temporary buffer */
            MPIU_Free((char *) tmp_buf + true_lb);
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ACCUMULATE);
    return mpi_errno;
}
