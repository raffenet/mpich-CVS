/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Accumulate(void *origin_addr, int origin_count, MPI_Datatype
                    origin_datatype, int target_rank, MPI_Aint target_disp,
                    int target_count, MPI_Datatype target_datatype, MPI_Op op,
                    MPID_Win *win_ptr)
{
    int nest_level_inc = FALSE;
    int mpi_errno=MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(2);
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_ACCUMULATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPID_ACCUMULATE);

    if (MPIDI_Use_optimized_rma) {
#       ifdef MPIDI_CH3_IMPLEMENTS_ACCUMULATE
        {
            mpi_errno = MPIDI_CH3_Accumulate(origin_addr, origin_count, origin_datatype, 
                                             target_rank, target_disp, target_count, target_datatype,
                                             op, win_ptr);
        }
#       endif
    }
    else {
        MPIDI_msg_sz_t data_sz;
        int dt_contig, rank;
        MPI_Aint dt_true_lb;
        MPIDI_RMA_ops *curr_ptr, *prev_ptr, *new_ptr;
        MPID_Datatype *dtp;
        MPIDI_Datatype_get_info(origin_count, origin_datatype,
                                dt_contig, data_sz, dtp, dt_true_lb);  

        if ((data_sz == 0) || (target_rank == MPI_PROC_NULL))
        {
            goto fn_exit;
        }

        MPIR_Nest_incr();
	nest_level_inc = TRUE;
	
        /* FIXME: It makes sense to save the rank (and size) of the
           communicator in the window structure to speed up these operations,
           or to save a pointer to the communicator structure, rather than
           just the handle 
        */
        NMPI_Comm_rank(win_ptr->comm, &rank);
        
        if (target_rank == rank)
        {
            MPI_User_function *uop;
            
            if (op == MPI_REPLACE)
            {
                mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
                                           (char *) win_ptr->base + win_ptr->disp_unit *
                                           target_disp, target_count, target_datatype); 
                goto fn_exit;
            }

	    MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN), mpi_errno, MPI_ERR_OP, "**opnotpredefined",
				 "**opnotpredefined %d", op );
	    
	    /* get the function by indexing into the op table */
	    uop = MPIR_Op_table[(op)%16 - 1];
            
            if (HANDLE_GET_KIND(origin_datatype) == HANDLE_KIND_BUILTIN &&
                HANDLE_GET_KIND(target_datatype) == HANDLE_KIND_BUILTIN)
            {    
                (*uop)(origin_addr, (char *) win_ptr->base + win_ptr->disp_unit *
                       target_disp, &target_count, &target_datatype);
            }
            else
            {
                /* derived datatype */
                
                MPID_Segment *segp;
                DLOOP_VECTOR *dloop_vec;
                MPI_Aint first, last;
                int vec_len, i, type_size, count;
                MPI_Datatype type;
                MPID_Datatype *dtp;
                MPI_Aint true_lb, true_extent, extent;
                void *tmp_buf=NULL, *source_buf, *target_buf;
                
                if (origin_datatype != target_datatype)
                {
                    /* first copy the data into a temporary buffer with
                       the same datatype as the target. Then do the
                       accumulate operation. */

                    mpi_errno = NMPI_Type_get_true_extent(target_datatype, 
                                                          &true_lb, &true_extent);
                    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**fail");
                    
                    MPID_Datatype_get_extent_macro(target_datatype, extent); 

		    MPIU_CHKLMEM_MALLOC(tmp_buf, void *, target_count * (MPIR_MAX(extent,true_extent)), mpi_errno,
					"temporary buffer");

                    /* --END ERROR HANDLING-- */
                    /* adjust for potential negative lower bound in datatype */
                    tmp_buf = (void *)((char*)tmp_buf - true_lb);
                    
                    mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
                                               origin_datatype, tmp_buf,
                                               target_count, target_datatype);  
                }
                
                segp = MPID_Segment_alloc();
		MPIU_ERR_CHKANDJUMP((!segp), mpi_errno, MPI_ERR_OTHER, "**nomem"); 
                MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
                first = 0;
                last  = SEGMENT_IGNORE_LAST;
                
                MPID_Datatype_get_ptr(target_datatype, dtp);
                vec_len = dtp->n_contig_blocks * target_count + 1; 
                /* +1 needed because Rob says so */
		MPIU_CHKLMEM_MALLOC(dloop_vec, DLOOP_VECTOR *, vec_len * sizeof(DLOOP_VECTOR), mpi_errno, "dloop vector");
                
                MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);
                
                source_buf = (tmp_buf != NULL) ? tmp_buf : origin_addr;
                target_buf = (char *) win_ptr->base + win_ptr->disp_unit * target_disp;
                type = dtp->eltype;
                type_size = MPID_Datatype_get_basic_size(type);
                for (i=0; i<vec_len; i++)
                {
                    count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
                    (*uop)((char *)source_buf + MPIU_PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                           (char *)target_buf + MPIU_PtrToInt( dloop_vec[i].DLOOP_VECTOR_BUF ),
                           &count, &type);
                }
                
                MPID_Segment_free(segp);
            }
        }
        else
        {
            /* queue it up */
            curr_ptr = win_ptr->rma_ops_list;
            prev_ptr = curr_ptr;
            while (curr_ptr != NULL)
            {
                prev_ptr = curr_ptr;
                curr_ptr = curr_ptr->next;
            }
            
	    MPIU_CHKPMEM_MALLOC(new_ptr, MPIDI_RMA_ops *, sizeof(MPIDI_RMA_ops), mpi_errno, "RMA operation entry");
            if (prev_ptr != NULL)
            {
                prev_ptr->next = new_ptr;
            }
            else
            {
                win_ptr->rma_ops_list = new_ptr;
            }
        
            new_ptr->next = NULL;  
            new_ptr->type = MPIDI_RMA_ACCUMULATE;
            new_ptr->origin_addr = origin_addr;
            new_ptr->origin_count = origin_count;
            new_ptr->origin_datatype = origin_datatype;
            new_ptr->target_rank = target_rank;
            new_ptr->target_disp = target_disp;
            new_ptr->target_count = target_count;
            new_ptr->target_datatype = target_datatype;
            new_ptr->op = op;
            
            /* if source or target datatypes are derived, increment their
               reference counts */ 
            if (HANDLE_GET_KIND(origin_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype_get_ptr(origin_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
            }
            if (HANDLE_GET_KIND(target_datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype_get_ptr(target_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
            }
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    if (nest_level_inc);
    { 
	MPIR_Nest_decr();
    }
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPID_ACCUMULATE);
    return mpi_errno;

  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}
