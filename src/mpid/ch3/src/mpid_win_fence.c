/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

MPIU_RMA_ops *MPIU_RMA_ops_list=NULL;

int MPID_Win_fence(int assert, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS, comm_size, *nops_to_proc, src, dest;
    int *nops_from_proc, rank, i, j, *tags, total_op_count, req_cnt;
    MPIU_RMA_ops *curr_ptr, *next_ptr;
    MPI_Comm comm;
    typedef struct MPIU_RMA_op_info {
        int type;
        MPI_Aint disp;
        int count;
        int datatype;
        MPI_Op op;
    } MPIU_RMA_op_info;
    MPIU_RMA_op_info *rma_op_infos;
    MPI_Request *reqs;
    MPI_User_function *uop;
    MPI_Op op;
    void *tmp_buf;
    MPI_Aint extent;

    MPIDI_STATE_DECL(MPID_STATE_MPID_WIN_FENCE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_WIN_FENCE);

    if (win_ptr->fence_cnt == 0) {
        /* This is the first call to fence. Do nothing except
           increment the count. */
        win_ptr->fence_cnt = 1;
    }
    else {
        /* This is the second fence. Do all the preceding RMA ops. */

	MPIR_Nest_incr();

        comm = win_ptr->comm;
        NMPI_Comm_size(comm, &comm_size);
        NMPI_Comm_rank(comm, &rank);

        /* First, each process informs every other process how many
           RMA ops from this process is it the target for. */

        nops_to_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_to_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }
        nops_from_proc = (int *) MPIU_Calloc(comm_size, sizeof(int));
        if (!nops_from_proc) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

        total_op_count = 0;
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            nops_to_proc[curr_ptr->target_rank]++;
            total_op_count++;
            curr_ptr = curr_ptr->next;
        }
        
        NMPI_Alltoall(nops_to_proc, 1, MPI_INT, nops_from_proc, 1,
                     MPI_INT, comm);

        /* For each RMA op, first send the type (put or get), target
           displ, count, datatype. Then issue an isend for a 
           put or irecv for a get. */

        rma_op_infos = (MPIU_RMA_op_info *) 
            MPIU_Malloc((total_op_count+1) * sizeof(MPIU_RMA_op_info));
        /* allocate one extra for use in receiving info below */ 

        if (!rma_op_infos) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

        reqs = (MPI_Request *)
            MPIU_Malloc(total_op_count*2*sizeof(MPI_Request)); 
        if (!reqs) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

        tags = (int *) MPIU_Calloc(comm_size, sizeof(int)); 
        if (!tags) {
            mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
            return mpi_errno;
        }

        i = 0;
        req_cnt = 0;
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            rma_op_infos[i].type = curr_ptr->type;
            rma_op_infos[i].disp = curr_ptr->target_disp;
            rma_op_infos[i].count = curr_ptr->target_count;
            rma_op_infos[i].datatype = curr_ptr->target_datatype;
            rma_op_infos[i].op = curr_ptr->op;
            
            dest = curr_ptr->target_rank;

            /* NEED TO CONVERT THE FOLLOWING TO USE STRUCT DATATYPE */
            mpi_errno = NMPI_Isend(&rma_op_infos[i],
                                   sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                   dest, tags[dest], comm,
                                   &reqs[req_cnt]); 
            if (mpi_errno) return mpi_errno;
            req_cnt++;
            tags[dest]++;
            if ((curr_ptr->type == MPID_REQUEST_PUT) ||
                (curr_ptr->type == MPID_REQUEST_ACCUMULATE))
                mpi_errno = NMPI_Isend(curr_ptr->origin_addr,
                                       curr_ptr->origin_count,
                                       curr_ptr->origin_datatype,
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
            else
                mpi_errno = NMPI_Irecv(curr_ptr->origin_addr,
                                       curr_ptr->origin_count,
                                       curr_ptr->origin_datatype,
                                       dest, tags[dest], comm,
                                       &reqs[req_cnt]); 
            if (mpi_errno) return mpi_errno;
            req_cnt++;
            tags[dest]++;

            curr_ptr = curr_ptr->next;
            i++;
        }        


        /* Now for each op for which this process is a target, first
           get the info regarding that op and then post an isend or
           irecv to perform the operation. */

        for (i=0; i<comm_size; i++) {
            /* instead of having everyone start receiving from 0,
               stagger the recvs a bit */ 
            src = (rank + i) % comm_size;
            tags[src] = 0;  /* start from tag 0 */

            for (j=0; j<nops_from_proc[src]; j++) {
                /* recv the info about the RMA op into the extra
                   MPIU_RMA_op_info allocated at the end of the
                   rma_op_infos array. */
                mpi_errno = NMPI_Recv(&rma_op_infos[total_op_count],
                                      sizeof(MPIU_RMA_op_info), MPI_BYTE, 
                                      src, tags[src], comm,
                                      MPI_STATUS_IGNORE);
                if (mpi_errno) return mpi_errno;
                tags[src]++;

                switch (rma_op_infos[total_op_count].type) {
                case MPID_REQUEST_PUT:
                    /* recv the put */
                    mpi_errno = NMPI_Recv((char *) win_ptr->base +
                                          win_ptr->disp_unit *
                                          rma_op_infos[total_op_count].disp,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) return mpi_errno;
                    break;
                case MPID_REQUEST_GET:
                    /* send the get */
                    mpi_errno = NMPI_Send((char *) win_ptr->base +
                                          win_ptr->disp_unit *
                                          rma_op_infos[total_op_count].disp,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm);
                    if (mpi_errno) return mpi_errno;
                    break;
                case MPID_REQUEST_ACCUMULATE:
                    /* recv the data into a temp buffer and perform
                       the reduction operation */
                    NMPI_Type_extent(rma_op_infos[total_op_count].datatype, 
                                     &extent); 
                    tmp_buf = MPIU_Malloc(extent * 
                                          rma_op_infos[total_op_count].count);
                    if (!tmp_buf) {
                        mpi_errno = MPIR_Err_create_code(
                            MPI_ERR_OTHER, "**nomem", 0 ); 
                        return mpi_errno;
                    }
                    mpi_errno = NMPI_Recv(tmp_buf,
                                          rma_op_infos[total_op_count].count,
                                          rma_op_infos[total_op_count].datatype,
                                          src, tags[src], comm,
                                          MPI_STATUS_IGNORE);
                    if (mpi_errno) return mpi_errno;

                    op = rma_op_infos[total_op_count].op;
                    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                        /* get the function by indexing into the op table */
                        uop = MPIR_Op_table[op%16 - 1];
                    }
                    else {
                        mpi_errno = MPIR_Err_create_code( MPI_ERR_OP,
                                                          "**opundefined","**opundefined %s", "only predefined ops valid for MPI_Accumulate" );
                        return mpi_errno;
                    }
                    (*uop)(tmp_buf, (char *) win_ptr->base +
                           win_ptr->disp_unit *
                           rma_op_infos[total_op_count].disp,
                           &(rma_op_infos[total_op_count].count),
                           &(rma_op_infos[total_op_count].datatype));
                    MPIU_Free(tmp_buf);
                    break;
                default:
                    mpi_errno = MPIR_Err_create_code( MPI_ERR_OP,
                                                      "****intern","**opundefined %s", "RMA target received unknown RMA operation" );
                    return mpi_errno;
                }

                tags[src]++;

            }
        }

        mpi_errno = NMPI_Waitall(req_cnt, reqs, MPI_STATUSES_IGNORE);
        if (mpi_errno) return mpi_errno;

	MPIR_Nest_decr();

        MPIU_Free(tags);
        MPIU_Free(reqs);
        MPIU_Free(rma_op_infos);
        MPIU_Free(nops_to_proc);
        MPIU_Free(nops_from_proc);

        /* free MPIU_RMA_ops_list */
        curr_ptr = MPIU_RMA_ops_list;
        while (curr_ptr != NULL) {
            next_ptr = curr_ptr->next;
            MPIU_Free(curr_ptr);
            curr_ptr = next_ptr;
        }
        MPIU_RMA_ops_list = NULL;

        /* Decrement the fence count. */
        win_ptr->fence_cnt = 0;

    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_WIN_FENCE);

    return mpi_errno;
}
