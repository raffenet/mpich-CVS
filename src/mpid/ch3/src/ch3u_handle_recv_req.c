/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

static int create_derived_datatype(MPID_Request * rreq, MPID_Datatype ** dtp);
static int do_accumulate_op(MPID_Request * rreq);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC * vc, MPID_Request * rreq, int * complete)
{
    static int in_routine = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIU_Assert(in_routine == FALSE);
    in_routine = TRUE;
    
    switch(rreq->dev.ca)
    {
	case MPIDI_CH3_CA_COMPLETE:
	{
	    /* FIXME: put ONC operations into their own completion action */
	    
	    if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_RECV)
	    {
                /* mark data transfer as complete and decrement CC */
		MPIDI_CH3U_Request_complete(rreq);
		*complete = TRUE;
	    }
            else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RESP)
	    {
                if (rreq->dev.win_ptr != NULL) {
                    /* atomically decrement RMA completion counter */
                    /* FIXME: MT: this has to be done atomically */
                    rreq->dev.win_ptr->my_counter -= 1;

                    /* grant next lock in lock queue if there is any */
                    if (rreq->dev.win_ptr->lock_queue != NULL)
                        mpi_errno = MPIDI_CH3I_Grant_next_lock(rreq->dev.win_ptr);
                    else
                        rreq->dev.win_ptr->current_lock_type = MPID_LOCK_NONE;
                }
		
                /* mark data transfer as complete and decrement CC */
		MPIDI_CH3U_Request_complete(rreq);
		*complete = TRUE;
            }
            else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP)
	    {
                /* accumulate data from tmp_buf into user_buf */
                mpi_errno = do_accumulate_op(rreq);
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */

                if (rreq->dev.win_ptr != NULL) {
                    /* atomically decrement RMA completion counter */
                    /* FIXME: MT: this has to be done atomically */
                    rreq->dev.win_ptr->my_counter -= 1;

                    /* grant next lock in lock queue if there is any */
                    if (rreq->dev.win_ptr->lock_queue != NULL)
                        mpi_errno = MPIDI_CH3I_Grant_next_lock(rreq->dev.win_ptr);
                    else
                        rreq->dev.win_ptr->current_lock_type = MPID_LOCK_NONE;
                }

                /* mark data transfer as complete and decrement CC */
		MPIDI_CH3U_Request_complete(rreq);
		*complete = TRUE;
            
            }
            else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RESP_DERIVED_DT)
	    {
                MPID_Datatype *new_dtp;
                
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* update request to get the data */
                MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_PUT_RESP);
                rreq->dev.datatype = new_dtp->handle;
                rreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                
                rreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. free dtype_info here. */
                MPIU_Free(rreq->dev.dtype_info);

                MPID_Segment_init(rreq->dev.user_buf,
                                  rreq->dev.user_count,
                                  rreq->dev.datatype,
                                  &rreq->dev.segment, 0);
                rreq->dev.segment_first = 0;
                rreq->dev.segment_size = rreq->dev.recv_data_sz;

                mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|loadrecviov", 0);
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */

		*complete = FALSE;
            }
            else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP_DERIVED_DT)
	    {
                MPID_Datatype *new_dtp;
                MPI_Aint true_lb, true_extent, extent;
                void *tmp_buf;
               
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);

                /* update new request to get the data */
                MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_ACCUM_RESP);

                /* first need to allocate tmp_buf to recv the data into */

		MPIR_Nest_incr();
                mpi_errno = NMPI_Type_get_true_extent(new_dtp->handle, 
                                                      &true_lb, &true_extent);
		MPIR_Nest_decr();
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */

                MPID_Datatype_get_extent_macro(new_dtp->handle, extent); 

                tmp_buf = MPIU_Malloc(rreq->dev.user_count * 
                                      (MPIR_MAX(extent,true_extent)));  
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

                rreq->dev.user_buf = tmp_buf;
                rreq->dev.datatype = new_dtp->handle;
                rreq->dev.recv_data_sz = new_dtp->size *
                                           rreq->dev.user_count; 
                rreq->dev.datatype_ptr = new_dtp;
                /* this will cause the datatype to be freed when the
                   request is freed. free dtype_info here. */
                MPIU_Free(rreq->dev.dtype_info);

                MPID_Segment_init(rreq->dev.user_buf,
                                  rreq->dev.user_count,
                                  rreq->dev.datatype,
                                  &rreq->dev.segment, 0);
                rreq->dev.segment_first = 0;
                rreq->dev.segment_size = rreq->dev.recv_data_sz;

                mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno != MPI_SUCCESS)
                {
                    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
						     "**ch3|loadrecviov", 0);
                    goto fn_exit;
                }
		/* --END ERROR HANDLING-- */

		*complete = FALSE;
            }
            else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_RESP_DERIVED_DT)
	    {
                MPID_Datatype *new_dtp;
                MPIDI_CH3_Pkt_t upkt;
                MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
                MPID_IOV iov[MPID_IOV_LIMIT];
		MPID_Request * sreq;
                int iov_n;
                
                /* create derived datatype */
                create_derived_datatype(rreq, &new_dtp);
                MPIU_Free(rreq->dev.dtype_info);

                /* create request for sending data */
		sreq = MPIDI_CH3_Request_create();
                sreq->kind = MPID_REQUEST_SEND;
                MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_RESP);
                sreq->dev.user_buf = rreq->dev.user_buf;
                sreq->dev.user_count = rreq->dev.user_count;
                sreq->dev.datatype = new_dtp->handle;
                sreq->dev.datatype_ptr = new_dtp;
		sreq->dev.win_ptr = rreq->dev.win_ptr;
		
                get_resp_pkt->type = MPIDI_CH3_PKT_GET_RESP;
                get_resp_pkt->request = rreq->dev.request;
                
                iov[0].MPID_IOV_BUF = (void*) get_resp_pkt;
                iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);

                MPID_Segment_init(sreq->dev.user_buf,
                                  sreq->dev.user_count,
                                  sreq->dev.datatype,
                                  &sreq->dev.segment, 0);
                sreq->dev.segment_first = 0;
		sreq->dev.segment_size = new_dtp->size * sreq->dev.user_count;

                iov_n = MPID_IOV_LIMIT - 1;
                mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &iov[1], &iov_n);
                if (mpi_errno == MPI_SUCCESS)
                {
                    iov_n += 1;
		
                    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iov_n);
		    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno != MPI_SUCCESS)
                    {
                        MPIU_Object_set_ref(sreq, 0);
                        MPIDI_CH3_Request_destroy(sreq);
                        sreq = NULL;
                        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
							 "**ch3|rmamsg", 0);
                        goto fn_exit;
                    }
		    /* --END ERROR HANDLING-- */
                }

                /* mark receive data transfer as complete and decrement CC in receive request */
		MPIDI_CH3U_Request_complete(rreq);
		*complete = TRUE;
            }
	    /* --BEGIN ERROR HANDLING-- */
	    else
	    {
		/* We shouldn't reach this code because the only other request types are sends */
		MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_RECV);
		MPIDI_CH3U_Request_complete(rreq);
		*complete = TRUE;
	    }
	    /* --END ERROR HANDLING-- */
	    
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_UEBUF_AND_COMPLETE:
	{
	    int recv_pending;
	    
            MPIDI_Request_recv_pending(rreq, &recv_pending);
	    if (!recv_pending)
	    { 
		if (rreq->dev.recv_data_sz > 0)
		{
		    MPIDI_CH3U_Request_unpack_uebuf(rreq);
		    MPIU_Free(rreq->dev.tmpbuf);
		}
	    }
	    else
	    {
		/* The receive has not been posted yet.  MPID_{Recv/Irecv}() is responsible for unpacking the buffer. */
	    }
	    
	    /* mark data transfer as complete and decrement CC */
	    MPIDI_CH3U_Request_complete(rreq);
	    *complete = TRUE;
	    
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_COMPLETE:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);

            if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RESP)
	    {
                if (rreq->dev.win_ptr != NULL) {
                    /* atomically decrement RMA completion counter */
                    /* FIXME: MT: this has to be done atomically */
                    rreq->dev.win_ptr->my_counter -= 1;

                    /* grant next lock in lock queue if there is any */
                    if (rreq->dev.win_ptr->lock_queue != NULL)
                        mpi_errno = MPIDI_CH3I_Grant_next_lock(rreq->dev.win_ptr);
                    else
                        rreq->dev.win_ptr->current_lock_type = MPID_LOCK_NONE;
                }
            }
            else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP)
	    {
                /* accumulate data from tmp_buf into user_buf */
                mpi_errno = do_accumulate_op(rreq);
		/* --BEGIN ERROR HANDLING-- */
                if (mpi_errno)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		    goto fn_exit;
		}
		/* --END ERROR HANDLING-- */

                if (rreq->dev.win_ptr != NULL) {
                    /* atomically decrement RMA completion counter */
                    /* FIXME: MT: this has to be done atomically */
                    rreq->dev.win_ptr->my_counter -= 1;

                    /* grant next lock in lock queue if there is any */
                    if (rreq->dev.win_ptr->lock_queue != NULL)
                        mpi_errno = MPIDI_CH3I_Grant_next_lock(rreq->dev.win_ptr);
                    else
                        rreq->dev.win_ptr->current_lock_type = MPID_LOCK_NONE;
                }
            }

	    /* mark data transfer as complete and decrement CC */
	    MPIDI_CH3U_Request_complete(rreq);
	    *complete = TRUE;
	    
	    break;
	}
	
	case MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV:
	{
	    MPIDI_CH3U_Request_unpack_srbuf(rreq);
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov",
						 "**ch3|loadrecviov %s", "MPIDI_CH3_CA_UNPACK_SRBUF_AND_RELOAD_IOV");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    *complete = FALSE;
	    break;
	}
	
	case MPIDI_CH3_CA_RELOAD_IOV:
	{
	    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|loadrecviov",
						 "**ch3|loadrecviov %s", "MPIDI_CH3_CA_RELOAD_IOV");
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	    *complete = FALSE;
	    break;
	}

	/* --BEGIN ERROR HANDLING-- */
	default:
	{
	    *complete = TRUE;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|badca",
					     "**ch3|badca %d", rreq->dev.ca);
	    break;
	}
	/* --END ERROR HANDLING-- */
    }

  fn_exit:
    in_routine = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);
    return mpi_errno;
}



static int create_derived_datatype(MPID_Request *req, MPID_Datatype **dtp)
{
    MPIDI_RMA_dtype_info *dtype_info;
    void *dataloop;
    MPID_Datatype *new_dtp;
    int mpi_errno=MPI_SUCCESS;
    MPI_Aint ptrdiff;

    dtype_info = req->dev.dtype_info;
    dataloop = req->dev.dataloop;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    *dtp = new_dtp;
            
    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 1;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->is_contig = dtype_info->is_contig;
    new_dtp->n_contig_blocks = dtype_info->n_contig_blocks; 
    new_dtp->size = dtype_info->size;
    new_dtp->extent = dtype_info->extent;
    new_dtp->dataloop_size = dtype_info->dataloop_size;
    new_dtp->dataloop_depth = dtype_info->dataloop_depth; 
    new_dtp->eltype = dtype_info->eltype;
    /* set dataloop pointer */
    new_dtp->dataloop = req->dev.dataloop;
    
    new_dtp->ub = dtype_info->ub;
    new_dtp->lb = dtype_info->lb;
    new_dtp->true_ub = dtype_info->true_ub;
    new_dtp->true_lb = dtype_info->true_lb;
    new_dtp->has_sticky_ub = dtype_info->has_sticky_ub;
    new_dtp->has_sticky_lb = dtype_info->has_sticky_lb;
    /* update pointers in dataloop */
    ptrdiff = (MPI_Aint)((char *) (new_dtp->dataloop) - (char *)
                         (dtype_info->dataloop));
    
    MPID_Dataloop_update(new_dtp->dataloop, ptrdiff);

    new_dtp->contents = NULL;

    return mpi_errno;
}


static int do_accumulate_op(MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_lb, true_extent;
    MPI_User_function *uop;

    if (rreq->dev.op == MPI_REPLACE)
    {
        /* simply copy the data */
        mpi_errno = MPIR_Localcopy(rreq->dev.user_buf, rreq->dev.user_count,
                                   rreq->dev.datatype,
                                   rreq->dev.real_user_buf,
                                   rreq->dev.user_count,
                                   rreq->dev.datatype);
	/* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    return mpi_errno;
	}
	/* --END ERROR HANDLING-- */
        goto fn_exit;
    }

    if (HANDLE_GET_KIND(rreq->dev.op) == HANDLE_KIND_BUILTIN)
    {
        /* get the function by indexing into the op table */
        uop = MPIR_Op_table[(rreq->dev.op)%16 - 1];
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opnotpredefined", "**opnotpredefined %d", rreq->dev.op );
        return mpi_errno;
	/* --END ERROR HANDLING-- */
    }
    
    if (HANDLE_GET_KIND(rreq->dev.datatype) == HANDLE_KIND_BUILTIN)
    {
        (*uop)(rreq->dev.user_buf, rreq->dev.real_user_buf,
               &(rreq->dev.user_count), &(rreq->dev.datatype));
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
        
        segp = MPID_Segment_alloc();
	/* --BEGIN ERROR HANDLING-- */
        if (!segp)
	{
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 ); 
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
        MPID_Segment_init(NULL, rreq->dev.user_count,
			  rreq->dev.datatype, segp, 0);
        first = 0;
        last  = SEGMENT_IGNORE_LAST;
        
        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        vec_len = dtp->n_contig_blocks * rreq->dev.user_count + 1; 
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
        for (i=0; i<vec_len; i++)
	{
            count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
            (*uop)((char *)rreq->dev.user_buf + POINTER_TO_AINT( dloop_vec[i].DLOOP_VECTOR_BUF ),
                   (char *)rreq->dev.real_user_buf + POINTER_TO_AINT( dloop_vec[i].DLOOP_VECTOR_BUF ),
                   &count, &type);
        }
        
        MPID_Segment_free(segp);
        MPIU_Free(dloop_vec);
    }

 fn_exit:
    /* free the temporary buffer */
    mpi_errno = NMPI_Type_get_true_extent(rreq->dev.datatype, 
                                          &true_lb, &true_extent);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    MPIU_Free((char *) rreq->dev.user_buf + true_lb);

    return mpi_errno;
}


int MPIDI_CH3I_Grant_next_lock(MPID_Win *win_ptr)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_granted_t * lock_granted_pkt = &upkt.lock_granted;
    MPID_Request *req;
    MPIDI_Win_lock_queue *curr_ptr;
    int mpi_errno = MPI_SUCCESS;

    curr_ptr = win_ptr->lock_queue;

    /* increment window counter */
    /* FIXME: MT: this has to be done atomically */
    win_ptr->my_counter++;

    /* set new lock type on window */
    win_ptr->current_lock_type = curr_ptr->lock_type;

    /* send lock granted packet */
    lock_granted_pkt->type = MPIDI_CH3_PKT_LOCK_GRANTED;
    lock_granted_pkt->lock_granted_flag_ptr = curr_ptr->lock_granted_flag_ptr;
                
    mpi_errno = MPIDI_CH3_iStartMsg(curr_ptr->vc, lock_granted_pkt,
                                    sizeof(*lock_granted_pkt), &req);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER,
                                         "**ch3|rmamsg", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    if (req != NULL)
    {
        MPID_Request_release(req);
    }

    win_ptr->lock_queue = curr_ptr->next;
    MPIU_Free(curr_ptr);

    return mpi_errno;
}
