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
    int mpi_errno = MPI_SUCCESS;
    MPIU_RMA_ops *curr_ptr, *prev_ptr, *new_ptr;
    MPID_Datatype *dtp;
    MPIDI_STATE_DECL(MPID_STATE_MPI_ACCUMULATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPI_ACCUMULATE);

    curr_ptr = MPIU_RMA_ops_list;
    prev_ptr = MPIU_RMA_ops_list;
    while (curr_ptr != NULL) {
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
    }

    new_ptr = (MPIU_RMA_ops *) MPIU_Malloc(sizeof(MPIU_RMA_ops));
    if (!new_ptr) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
        MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);
        return mpi_errno;
    }
    if (prev_ptr != NULL)
        prev_ptr->next = new_ptr;
    else 
        MPIU_RMA_ops_list = new_ptr;

    new_ptr->next = NULL;  
    new_ptr->type = MPID_REQUEST_ACCUMULATE;
    new_ptr->origin_addr = origin_addr;
    new_ptr->origin_count = origin_count;
    new_ptr->origin_datatype = origin_datatype;
    new_ptr->target_rank = target_rank;
    new_ptr->target_disp = target_disp;
    new_ptr->target_count = target_count;
    new_ptr->target_datatype = target_datatype;
    new_ptr->op = op;

    if (HANDLE_GET_KIND(origin_datatype) != HANDLE_KIND_BUILTIN) {
        MPID_Datatype_get_ptr(origin_datatype, dtp);
        MPID_Datatype_add_ref(dtp);
    }
    if (HANDLE_GET_KIND(target_datatype) != HANDLE_KIND_BUILTIN) {
        MPID_Datatype_get_ptr(target_datatype, dtp);
        MPID_Datatype_add_ref(dtp);
    }

    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPI_ACCUMULATE);

    return mpi_errno;
}
