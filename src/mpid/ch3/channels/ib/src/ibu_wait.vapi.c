/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ibu.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include "mpidi_ch3_impl.h"

#ifdef USE_IB_VAPI

#include "ibuimpl.vapi.h"

/*#define PRINT_IBU_WAIT*/
#ifdef PRINT_IBU_WAIT
#define MPIU_DBG_PRINTFX(a) MPIU_DBG_PRINTF(a)
#else
#define MPIU_DBG_PRINTFX(a)
#endif

char * op2str(int opcode)
{
    static char str[20];
    switch(opcode)
    {
    case VAPI_CQE_SQ_SEND_DATA:
	return "VAPI_CQE_SQ_SEND_DATA";
    case VAPI_CQE_SQ_RDMA_WRITE:
	return "VAPI_CQE_SQ_RDMA_WRITE";
    case VAPI_CQE_SQ_RDMA_READ:
	return "VAPI_CQE_SQ_RDMA_READ";
    case VAPI_CQE_SQ_COMP_SWAP:
	return "VAPI_CQE_SQ_COMP_SWAP";
    case VAPI_CQE_SQ_FETCH_ADD:
	return "VAPI_CQE_SQ_FETCH_ADD";
    case VAPI_CQE_SQ_BIND_MRW:
	return "VAPI_CQE_SQ_BIND_MRW";
    case VAPI_CQE_RQ_SEND_DATA:
	return "VAPI_CQE_RQ_SEND_DATA";
    case VAPI_CQE_RQ_RDMA_WITH_IMM:
	return "VAPI_CQE_RQ_RDMA_WITH_IMM";
    case VAPI_CQE_INVAL_OPCODE:
	return "VAPI_CQE_INVAL_OPCODE";
    }
    sprintf(str, "%d", opcode);
    return str;
}

void PrintWC(VAPI_wc_desc_t *p)
{
    printf("Work Completion Descriptor:\n");
    printf(" id: %d\n", (int)p->id);
    printf(" opcode: %u = %s\n",
	   p->opcode, VAPI_cqe_opcode_sym(p->opcode));
    printf(" byte_len: %d\n", p->byte_len);
    printf(" imm_data_valid: %d\n", (int)p->imm_data_valid);
    printf(" imm_data: %u\n", (unsigned int)p->imm_data);
    printf(" remote_node_addr:\n");
    printf("  type: %u = %s\n",
	   p->remote_node_addr.type,
	   VAPI_remote_node_addr_sym(p->remote_node_addr.type));
    printf("  slid: %d\n", (int)p->remote_node_addr.slid);
    printf("  sl: %d\n", (int)p->remote_node_addr.sl);
    printf("  qp: %d\n", (int)p->remote_node_addr.qp_ety.qp);
    printf("  loc_eecn: %d\n", (int)p->remote_node_addr.ee_dlid.loc_eecn);
    printf(" grh_flag: %d\n", (int)p->grh_flag);
    printf(" pkey_ix: %d\n", p->pkey_ix);
    printf(" status: %u = %s\n",
	   (int)p->status, VAPI_wc_status_sym(p->status));
    printf(" vendor_err_syndrome: %d\n", p->vendor_err_syndrome);
    printf(" free_res_count: %d\n", p->free_res_count);
    fflush(stdout);
}

#undef FUNCNAME
#define FUNCNAME ibu_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_wait(ibu_set_t set, int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr)
{
    VAPI_ret_t status;
    VAPI_wc_desc_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    ibu_t ibu;
    int num_bytes;
    unsigned int offset;
    ibu_work_id_handle_t *id_ptr;
    int send_length;
#ifdef USE_INLINE_PKT_RECEIVE
    MPIDI_VC_t *recv_vc_ptr;
    void *mem_ptr_orig;
    int mpi_errno;
    int pkt_offset;
#ifdef MPIDI_CH3_CHANNEL_RNDV
    MPID_Request *sreq, *rreq;
    int complete;
    int i;
#endif
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WAIT);
    MPIU_DBG_PRINTFX(("entering ibu_wait\n"));
    for (;;) 
    {
	if (IBU_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes",
			      IBU_Process.unex_finished_list->read.total));
	    /* remove this ibu from the finished list */
	    ibu = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = IBU_Process.unex_finished_list->unex_finished_queue;
	    ibu->unex_finished_queue = NULL;

	    *num_bytes_ptr = ibu->read.total;
	    *op_ptr = IBU_OP_READ;
	    *vc_pptr = ibu->vc_ptr;
	    ibu->pending_operations--;
	    if (ibu->closing && ibu->pending_operations == 0)
	    {
		ibu = IBU_INVALID_QP;
	    }
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 1\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return MPI_SUCCESS;
	}

	status = VAPI_poll_cq(
	    IBU_Process.hca_handle,
	    set,
	    &completion_data);
	if (status == VAPI_EAGAIN || status == VAPI_CQ_EMPTY)
	{
	    /*usleep(1);*/
	    /* poll until there is something in the queue */
	    /* or the timeout has expired */
	    if (millisecond_timeout == 0)
	    {
		*num_bytes_ptr = 0;
		*vc_pptr = NULL;
		*op_ptr = IBU_OP_TIMEOUT;
		MPIU_DBG_PRINTFX(("exiting ibu_wait 2\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return MPI_SUCCESS;
	    }
	    continue;
	}
	if (status != VAPI_OK)
	{
	    /*MPIU_Internal_error_printf("%s: error: VAPI_poll_cq did not return VAPI_OK, %s\n", FCNAME, VAPI_strerror(status));*/
	    /*MPIU_dump_dbg_memlog_to_stdout();*/
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", VAPI_strerror(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 3\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return mpi_errno;
	}
	/*
	if (completion_data.status != VAPI_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: status = %s != VAPI_SUCCESS\n", 
		FCNAME, VAPI_strerror(completion_data.status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

	id_ptr = *((ibu_work_id_handle_t**)&completion_data.id);
	ibu = id_ptr->ibu;
	mem_ptr = (void*)(id_ptr->mem);
	send_length = id_ptr->length;
	ibuBlockFree(IBU_Process.workAllocator, (void*)id_ptr);
#ifdef USE_INLINE_PKT_RECEIVE
	mem_ptr_orig = mem_ptr;
#endif
	switch (completion_data.opcode)
	{
#ifdef MPIDI_CH3_CHANNEL_RNDV
	case VAPI_CQE_SQ_RDMA_WRITE:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
                    FCNAME, VAPI_wc_status_sym(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", VAPI_wc_status_sym(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d, g_pr: %d, g_ps: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends, g_pr, g_ps));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    /*if (ibu->state & IBU_RDMA_WRITING)*/
	    {
		MPI_Request rreq_cached;
		int complete = 0;
		ibu->state &= ~IBU_RDMA_WRITING;
		sreq = (MPID_Request*)mem_ptr;
		MPIU_DBG_PRINTF(("sreq after rdma write: sreq=0x%x, rreq=0x%x\n", sreq->handle, sreq->dev.rdma_request));
		rreq_cached = sreq->dev.rdma_request;
		if (sreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER)
		{
		    MPIU_DBG_PRINTF(("unregistering and reloading the sender's iov.\n"));
		    /* unpin the sender's iov */
		    for (i=0; i<sreq->dev.iov_count; i++)
		    {
			ibu_deregister_memory(sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN, &sreq->ch.local_iov_mem[i]);
		    }
		    /* update the sender's request */
		    mpi_errno = MPIDI_CH3U_Handle_send_req(ibu->vc_ptr, sreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after rdma write");
			MPIU_DBG_PRINTFX(("exiting ibu_wait a\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		if ((sreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER) || complete)
		{
		    MPIDI_CH3_Pkt_t pkt;
		    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
		    MPID_Request *reload_sreq = NULL;

		    MPIU_DBG_PRINTF(("sending a reload/done packet (sreq=0x%x, rreq=0x%x).\n", sreq->handle, rreq_cached));
		    /* send the reload/done packet to the receiver */
		    MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
		    reload_pkt->rreq = rreq_cached/*sreq->dev.rdma_request*/;
		    reload_pkt->sreq = sreq->handle;
		    reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
		    mpi_errno = MPIDI_CH3_iStartMsg(ibu->vc_ptr, reload_pkt, sizeof(*reload_pkt), &reload_sreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIU_DBG_PRINTFX(("exiting ibu_wait b\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
		    if (reload_sreq != NULL)
		    {
			/* The sender doesn't need to know when the packet has been sent.  So release the request immediately */
			MPID_Request_release(reload_sreq);
		    }
		}
		if (sreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER && !complete)
		{
		    /* pin the sender's iov */
		    MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
		    for (i=0; i<sreq->dev.iov_count; i++)
		    {
			ibu_register_memory(sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN, &sreq->ch.local_iov_mem[i]);
		    }
		}
		if ((!complete) && !(sreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER))
		{
		    sreq->ch.reload_state = 0;
		    /* do some more rdma writes */
		    mpi_errno = MPIDI_CH3I_rdma_writev(ibu->vc_ptr, sreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIU_DBG_PRINTFX(("exiting ibu_wait c\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
		}
		else
		{
		    sreq->ch.reload_state = 0;
		    /* return from the wait */
		    *num_bytes_ptr = 0;
		    *vc_pptr = ibu->vc_ptr;
		    *op_ptr = IBU_OP_WAKEUP;
		    MPIU_DBG_PRINTFX(("exiting ibu_wait d\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return MPI_SUCCESS;
		}
	    }
	    break;
	case VAPI_CQE_SQ_RDMA_READ:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
                    FCNAME, VAPI_wc_status_sym(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", VAPI_wc_status_sym(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d, g_pr: %d, g_ps: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends, g_pr, g_ps));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    /*if (ibu->state & IBU_RDMA_READING)*/
	    {
		MPI_Request sreq_cached;
		int complete = 0;
		ibu->state &= ~IBU_RDMA_READING;
		rreq = (MPID_Request*)mem_ptr;
		MPIU_DBG_PRINTF(("rreq after rdma read: rreq=0x%x, sreq=0x%x\n", rreq->handle, rreq->dev.rdma_request));
		sreq_cached = rreq->dev.rdma_request;
		if (rreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER)
		{
		    MPIU_DBG_PRINTF(("unregistering and reloading the receiver's iov.\n"));
		    /* unpin the receiver's iov */
		    for (i=0; i<rreq->dev.iov_count; i++)
		    {
			ibu_deregister_memory(rreq->dev.iov[i].MPID_IOV_BUF,
					      rreq->dev.iov[i].MPID_IOV_LEN,
					      &rreq->ch.local_iov_mem[i]);
		    }
		    /* update the receiver's request */
		    mpi_errno = MPIDI_CH3U_Handle_recv_req(ibu->vc_ptr, rreq, &complete);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after rdma read");
			MPIU_DBG_PRINTFX(("exiting ibu_wait e\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		if ((rreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER) || complete)
		{
		    MPIDI_CH3_Pkt_t pkt;
		    MPIDI_CH3_Pkt_rdma_reload_t * reload_pkt = &pkt.reload;
		    MPID_Request *reload_rreq = NULL;

		    MPIU_DBG_PRINTF(("sending a reload/done packet (sreq=0x%x, rreq=0x%x).\n", rreq->handle, sreq_cached));
		    /* send the reload/done packet to the sender */
		    MPIDI_Pkt_init(reload_pkt, MPIDI_CH3_PKT_RELOAD);
		    reload_pkt->sreq = sreq_cached/*rreq->dev.rdma_request*/;
		    reload_pkt->rreq = rreq->handle;
		    reload_pkt->send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
		    mpi_errno = MPIDI_CH3_iStartMsg(ibu->vc_ptr, reload_pkt, sizeof(*reload_pkt), &reload_rreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIU_DBG_PRINTFX(("exiting ibu_wait f\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
		    if (reload_rreq != NULL)
		    {
			/* The sender doesn't need to know when the packet has been sent.
			   So release the request immediately */
			MPID_Request_release(reload_rreq);
		    }
		}
		if (rreq->ch.reload_state & MPIDI_CH3I_RELOAD_RECEIVER && !complete)
		{
		    /* pin the receiver's iov */
		    MPIU_DBG_PRINTF(("registering the receiver's iov.\n"));
		    for (i=0; i<rreq->dev.iov_count; i++)
		    {
			ibu_register_memory(rreq->dev.iov[i].MPID_IOV_BUF,
					    rreq->dev.iov[i].MPID_IOV_LEN,
					    &rreq->ch.local_iov_mem[i]);
		    }
		}
		if ((!complete) && !(rreq->ch.reload_state & MPIDI_CH3I_RELOAD_SENDER))
		{
		    rreq->ch.reload_state = 0;
		    /* do some more rdma reads */
		    mpi_errno = MPIDI_CH3I_rdma_readv(ibu->vc_ptr, rreq);
		    /* --BEGIN ERROR HANDLING-- */
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			MPIU_DBG_PRINTFX(("exiting ibu_wait g\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		    /* --END ERROR HANDLING-- */
		}
		else
		{
		    rreq->ch.reload_state = 0;
		    /* return from the wait */
		    *num_bytes_ptr = 0;
		    *vc_pptr = ibu->vc_ptr;
		    *op_ptr = IBU_OP_WAKEUP;
		    MPIU_DBG_PRINTFX(("exiting ibu_wait h\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return MPI_SUCCESS;
		}
	    }
	    break;
#endif /* MPIDI_CH3_CHANNEL_RNDV */
	case VAPI_CQE_SQ_SEND_DATA:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: send completion status = %s\n",
                    FCNAME, VAPI_wc_status_sym(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", VAPI_wc_status_sym(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d, g_pr: %d, g_ps: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends, g_pr, g_ps));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 40\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    MPIU_DBG_PRINTF(("", g_ps--));
	    if (mem_ptr == (void*)-1)
	    {
		MPIU_DBG_PRINTF(("ack sent\n"));
		/* flow control ack completed, no user data so break out here */
		MPIU_DBG_PRINTF(("ack sent.\n"));
		break;
	    }
	    MPIU_DBG_PRINTF(("___total_s: %d\n", ++g_num_sent));
	    num_bytes = send_length;
	    MPIDI_DBG_PRINTF((60, FCNAME, "send num_bytes = %d\n", num_bytes));
	    ibuBlockFreeIB(ibu->allocator, mem_ptr);

	    *num_bytes_ptr = num_bytes;
	    *op_ptr = IBU_OP_WRITE;
	    *vc_pptr = ibu->vc_ptr;
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 5\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return MPI_SUCCESS;
	    break;
	case VAPI_CQE_RQ_SEND_DATA:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: recv completion status = %s\n",
                    FCNAME, VAPI_wc_status_sym(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", VAPI_wc_status_sym(completion_data.status));
		PrintWC(&completion_data);
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d, g_pr: %d, g_ps: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends, g_pr, g_ps));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 41\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    MPIU_DBG_PRINTF(("", g_pr--));
	    if (completion_data.imm_data_valid)
	    {
		ibu->nAvailRemote += completion_data.imm_data;
		MPIDI_DBG_PRINTF((60, FCNAME, "%d packets acked, nAvailRemote now = %d", completion_data.imm_data, ibu->nAvailRemote));
		ibuBlockFreeIB(ibu->allocator, mem_ptr);
		ibui_post_receive_unacked(ibu);
		assert(completion_data.byte_len == 1); /* check this after the printfs to see if the immediate data is correct */
		break;
	    }
	    MPIU_DBG_PRINTF(("___total_r: %d\n", ++g_num_received));
	    num_bytes = completion_data.byte_len;
#ifdef USE_INLINE_PKT_RECEIVE
	    recv_vc_ptr = ibu->vc_ptr;
	    pkt_offset = 0;
	    if (recv_vc_ptr->ch.reading_pkt)
	    {
#ifdef MPIDI_CH3_CHANNEL_RNDV
		if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type > MPIDI_CH3_PKT_END_CH3)
		{
		    if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RTS_IOV)
		    {
			MPIU_DBG_PRINTF(("received rts packet(sreq=0x%x).\n",
					 ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->sreq));
			rreq = MPID_Request_create();
			if (rreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait h\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			MPIU_Object_set_ref(rreq, 1);
			rreq->kind = MPIDI_CH3I_RTS_IOV_READ_REQUEST;
			rreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->sreq;
			rreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_rts_iov_t*)mem_ptr)->iov_len;
			rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->dev.rdma_iov;
			rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.remote_iov_mem[0];
			rreq->dev.iov[1].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			rreq->dev.iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.pkt;
			rreq->dev.iov[2].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
			rreq->dev.iov_count = 3;
			rreq->ch.req = NULL;
			recv_vc_ptr->ch.recv_active = rreq;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RTS_PUT)
		    {
			int found;
			MPIU_DBG_PRINTF(("received rts put packet(sreq=0x%x).\n",
					 ((MPIDI_CH3_Pkt_rndv_req_to_send_t*)mem_ptr)->sender_req_id));

			mpi_errno = MPIDI_CH3U_Handle_recv_rndv_pkt(recv_vc_ptr,
								    (MPIDI_CH3_Pkt_t*)mem_ptr,
								    &rreq, &found);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ibu read progress unable to handle incoming rts(put) packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait v\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			if (found)
			{
			    mpi_errno = MPIDI_CH3U_Post_data_receive(recv_vc_ptr, found, &rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			    mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}

			recv_vc_ptr->ch.recv_active = NULL;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_CTS_IOV)
		    {
			MPIU_DBG_PRINTF(("received cts packet(sreq=0x%x, rreq=0x%x).\n",
					 ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->sreq,
					 ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->rreq));
			MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->sreq, sreq);
			sreq->dev.rdma_request = ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->rreq;
			sreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_cts_iov_t*)mem_ptr)->iov_len;
			rreq = MPID_Request_create();
			if (rreq == NULL)
			{
			    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait i\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			MPIU_Object_set_ref(rreq, 1);
			rreq->kind = MPIDI_CH3I_IOV_WRITE_REQUEST;
			rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.rdma_iov;
			rreq->dev.iov[0].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.remote_iov_mem[0];
			rreq->dev.iov[1].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			rreq->dev.iov_count = 2;
			rreq->ch.req = sreq;
			recv_vc_ptr->ch.recv_active = rreq;
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_IOV)
		    {
			if ( ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_SEND )
			{
			    MPIU_DBG_PRINTF(("received sender's iov packet, posting a read of %d iovs.\n", ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->req, sreq);
			    sreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len;
			    rreq = MPID_Request_create();
			    if (rreq == NULL)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
				MPIU_DBG_PRINTFX(("exiting ibu_wait j\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    MPIU_Object_set_ref(rreq, 1);
			    rreq->kind = MPIDI_CH3I_IOV_READ_REQUEST;
			    rreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->dev.rdma_iov;
			    rreq->dev.iov[0].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			    rreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.remote_iov_mem[0];
			    rreq->dev.iov[1].MPID_IOV_LEN = sreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			    rreq->dev.iov_count = 2;
			    rreq->ch.req = sreq;
			    recv_vc_ptr->ch.recv_active = rreq;
			}
			else if ( ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_RECV )
			{
			    MPIU_DBG_PRINTF(("received receiver's iov packet, posting a read of %d iovs.\n", ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->req, rreq);
			    rreq->dev.rdma_iov_count = ((MPIDI_CH3_Pkt_rdma_iov_t*)mem_ptr)->iov_len;
			    sreq = MPID_Request_create();
			    if (sreq == NULL)
			    {
				mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
				MPIU_DBG_PRINTFX(("exiting ibu_wait k\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    MPIU_Object_set_ref(sreq, 1);
			    sreq->kind = MPIDI_CH3I_IOV_WRITE_REQUEST;
			    sreq->dev.iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->dev.rdma_iov;
			    sreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(MPID_IOV);
			    sreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.remote_iov_mem[0];
			    sreq->dev.iov[1].MPID_IOV_LEN = rreq->dev.rdma_iov_count * sizeof(ibu_mem_t);
			    sreq->dev.iov_count = 2;
			    sreq->ch.req = rreq;
			    recv_vc_ptr->ch.recv_active = sreq;
			}
			else
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "received invalid MPIDI_CH3_PKT_IOV packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait l\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
		    }
		    else if (((MPIDI_CH3_Pkt_t*)mem_ptr)->type == MPIDI_CH3_PKT_RELOAD)
		    {
			if (((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_SEND)
			{
			    MPIU_DBG_PRINTF(("received reload send packet (sreq=0x%x).\n", ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq, sreq);
			    MPIU_DBG_PRINTF(("unregistering the sender's iov.\n"));
			    for (i=0; i<sreq->dev.iov_count; i++)
			    {
				ibu_deregister_memory(sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN,
						      &sreq->ch.local_iov_mem[i]);
			    }
			    mpi_errno = MPIDI_CH3U_Handle_send_req(recv_vc_ptr, sreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update send request after receiving a reload packet");
				MPIU_DBG_PRINTFX(("exiting ibu_wait m\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    if (!complete)
			    {
				/* send a new iov */
				MPID_Request * rts_sreq;
				MPIDI_CH3_Pkt_t pkt;

				MPIU_DBG_PRINTF(("registering the sender's iov.\n"));
				for (i=0; i<sreq->dev.iov_count; i++)
				{
				    ibu_register_memory(sreq->dev.iov[i].MPID_IOV_BUF, sreq->dev.iov[i].MPID_IOV_LEN,
							&sreq->ch.local_iov_mem[i]);
				}
				MPIU_DBG_PRINTF(("sending reloaded send iov of length %d\n", sreq->dev.iov_count));
				MPIDI_Pkt_init(&pkt.iov, MPIDI_CH3_PKT_IOV);
				pkt.iov.send_recv = MPIDI_CH3_PKT_RELOAD_SEND;
				pkt.iov.req = ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq;
				pkt.iov.iov_len = sreq->dev.iov_count;

				sreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
				sreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
				sreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)sreq->dev.iov;
				sreq->dev.rdma_iov[1].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(MPID_IOV);
				sreq->dev.rdma_iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&sreq->ch.local_iov_mem[0];
				sreq->dev.rdma_iov[2].MPID_IOV_LEN = sreq->dev.iov_count * sizeof(ibu_mem_t);

				mpi_errno = MPIDI_CH3_iStartMsgv(recv_vc_ptr, sreq->dev.rdma_iov, 3, &rts_sreq);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    MPIU_Object_set_ref(sreq, 0);
				    MPIDI_CH3_Request_destroy(sreq);
				    sreq = NULL;
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|rtspkt", 0);
				    MPIU_DBG_PRINTFX(("exiting ibu_wait n\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
				if (rts_sreq != NULL)
				{
				    /* The sender doesn't need to know when the message has been sent.  So release the request immediately */
				    MPID_Request_release(rts_sreq);
				}
			    }
			}
			else if (((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->send_recv == MPIDI_CH3_PKT_RELOAD_RECV)
			{
			    MPIU_DBG_PRINTF(("received reload recv packet (rreq=0x%x).\n", ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq));
			    MPID_Request_get_ptr(((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->rreq, rreq);
			    MPIU_DBG_PRINTF(("unregistering the receiver's iov.\n"));
			    for (i=0; i<rreq->dev.iov_count; i++)
			    {
				ibu_deregister_memory(rreq->dev.iov[i].MPID_IOV_BUF, rreq->dev.iov[i].MPID_IOV_LEN,
						      &rreq->ch.local_iov_mem[i]);
			    }
			    mpi_errno = MPIDI_CH3U_Handle_recv_req(recv_vc_ptr, rreq, &complete);
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "unable to update request after receiving a reload packet");
				MPIU_DBG_PRINTFX(("exiting ibu_wait o\n"));
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    if (!complete)
			    {
				/* send a new iov */
				MPID_Request * cts_sreq;
				MPIDI_CH3_Pkt_t pkt;

				MPIU_DBG_PRINTF(("registering the receiver's iov.\n"));
				for (i=0; i<rreq->dev.iov_count; i++)
				{
				    ibu_register_memory(rreq->dev.iov[i].MPID_IOV_BUF, rreq->dev.iov[i].MPID_IOV_LEN,
							&rreq->ch.local_iov_mem[i]);
				}
				MPIU_DBG_PRINTF(("sending reloaded recv iov of length %d\n", rreq->dev.iov_count));
				MPIDI_Pkt_init(&pkt.iov, MPIDI_CH3_PKT_IOV);
				pkt.iov.send_recv = MPIDI_CH3_PKT_RELOAD_RECV;
				pkt.iov.req = ((MPIDI_CH3_Pkt_rdma_reload_t*)mem_ptr)->sreq;
				pkt.iov.iov_len = rreq->dev.iov_count;

				rreq->dev.rdma_iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&pkt;
				rreq->dev.rdma_iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
				rreq->dev.rdma_iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)rreq->dev.iov;
				rreq->dev.rdma_iov[1].MPID_IOV_LEN = rreq->dev.iov_count * sizeof(MPID_IOV);
				rreq->dev.rdma_iov[2].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)&rreq->ch.local_iov_mem[0];
				rreq->dev.rdma_iov[2].MPID_IOV_LEN = rreq->dev.iov_count * sizeof(ibu_mem_t);

				mpi_errno = MPIDI_CH3_iStartMsgv(recv_vc_ptr, rreq->dev.rdma_iov, 3, &cts_sreq);
				/* --BEGIN ERROR HANDLING-- */
				if (mpi_errno != MPI_SUCCESS)
				{
				    /* This destruction probably isn't correct. */
				    /* I think it needs to save the error in the request, complete the request and return */
				    MPIU_Object_set_ref(rreq, 0);
				    MPIDI_CH3_Request_destroy(rreq);
				    rreq = NULL;
				    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ch3|ctspkt", 0);
				    MPIU_DBG_PRINTFX(("exiting ibu_wait p\n"));
				    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				    return mpi_errno;
				}
				/* --END ERROR HANDLING-- */
				if (cts_sreq != NULL)
				{
				    /* The sender doesn't need to know when the message has been sent.  So release the request immediately */
				    MPID_Request_release(cts_sreq);
				}
			    }
			}
			else
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait q\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;
			if (num_bytes > sizeof(MPIDI_CH3_Pkt_t))
			{
			    MPIU_DBG_PRINTF(("pkt handled with %d bytes remaining to be buffered.\n", num_bytes));
			    ibui_buffer_unex_read(ibu, mem_ptr_orig, sizeof(MPIDI_CH3_Pkt_t), num_bytes);
			}
			else
			{
			    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
			    ibui_post_receive(ibu);
			}
			/* return from the wait */
			*num_bytes_ptr = 0;
			*vc_pptr = recv_vc_ptr;
			*op_ptr = IBU_OP_WAKEUP;
			MPIU_DBG_PRINTFX(("exiting ibu_wait q\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return MPI_SUCCESS;
		    }
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "shared memory read progress unable to handle unknown rdma packet");
			MPIU_DBG_PRINTFX(("exiting ibu_wait r\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		else
#endif /* MPIDI_CH3_CHANNEL_RNDV */
		{
		    mpi_errno = MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, (MPIDI_CH3_Pkt_t*)mem_ptr, &recv_vc_ptr->ch.recv_active);
		    if (mpi_errno != MPI_SUCCESS)
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "infiniband read progress unable to handle incoming packet");
			MPIU_DBG_PRINTFX(("exiting ibu_wait s\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    MPIU_DBG_PRINTF(("packet %d with no data handled.\n", g_num_received));
		    recv_vc_ptr->ch.reading_pkt = TRUE;
		}
		else
		{
		    /*
		    int z;
		    printf("posting a read of %d buffers\n",
			   recv_vc_ptr->ch.recv_active->dev.iov_count);
		    for (z=0; z<recv_vc_ptr->ch.recv_active->dev.iov_count; z++)
		    {
			printf(" [%d].len = %d\n", z, recv_vc_ptr->ch.recv_active->dev.iov[z].MPID_IOV_LEN);
		    }
		    fflush(stdout);
		    */
		    /*mpi_errno =*/ ibu_post_readv(ibu, recv_vc_ptr->ch.recv_active->dev.iov, recv_vc_ptr->ch.recv_active->dev.iov_count);
		}
		mem_ptr = (unsigned char *)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		num_bytes -= sizeof(MPIDI_CH3_Pkt_t);

		if (num_bytes == 0)
		{
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
		    ibui_post_receive(ibu);
		    break;
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    MPIU_DBG_PRINTF(("pkt handled with %d bytes remaining to be buffered.\n", num_bytes));
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, sizeof(MPIDI_CH3_Pkt_t), num_bytes);
		    break;
		}
		pkt_offset = sizeof(MPIDI_CH3_Pkt_t);
	    }
#endif
	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "ibu_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(ibu->state & IBU_READING))
	    {
#ifdef USE_INLINE_PKT_RECEIVE
		MPIU_DBG_PRINTF(("a:buffering %d bytes.\n", num_bytes));
		ibui_buffer_unex_read(ibu, mem_ptr_orig, pkt_offset, num_bytes);
#else
		ibui_buffer_unex_read(ibu, mem_ptr, 0, num_bytes);
#endif
		break;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", ibu->read.total, num_bytes, ibu->read.total + num_bytes));
	    if (ibu->read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && ibu->read.iovlen > 0)
		{
		    if ((int)ibu->read.iov[ibu->read.index].MPID_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, iter_ptr,
			    ibu->read.iov[ibu->read.index].MPID_IOV_LEN);
			iter_ptr += ibu->read.iov[ibu->read.index].MPID_IOV_LEN;
			/* update the iov */
			num_bytes -= ibu->read.iov[ibu->read.index].MPID_IOV_LEN;
			ibu->read.index++;
			ibu->read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].MPID_IOV_BUF, iter_ptr, num_bytes);
			iter_ptr += num_bytes;
			/* update the iov */
			ibu->read.iov[ibu->read.index].MPID_IOV_LEN -= num_bytes;
			ibu->read.iov[ibu->read.index].MPID_IOV_BUF = 
			    (char*)(ibu->read.iov[ibu->read.index].MPID_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (unsigned int)((unsigned char*)iter_ptr - (unsigned char*)mem_ptr);
		ibu->read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the receive packet back in the pool */
		    if (mem_ptr == NULL)
			MPIU_Internal_error_printf("ibu_wait: read mem_ptr == NULL\n");
		    assert(mem_ptr != NULL);
#ifdef USE_INLINE_PKT_RECEIVE
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
#else
		    ibuBlockFreeIB(ibu->allocator, mem_ptr);
#endif
		    ibui_post_receive(ibu);
		}
		else
		{
		    /* save the unused but received data */
#ifdef USE_INLINE_PKT_RECEIVE
		    MPIU_DBG_PRINTF(("b:buffering %d bytes (offset,pkt = %d,%d).\n", num_bytes, offset, pkt_offset));
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, offset + pkt_offset, num_bytes);
#else
		    ibui_buffer_unex_read(ibu, mem_ptr, offset, num_bytes);
#endif
		}
		if (ibu->read.iovlen == 0)
		{
		    if (recv_vc_ptr->ch.recv_active->kind < MPID_LAST_REQUEST_KIND)
		    {
			ibu->state &= ~IBU_READING;
			*num_bytes_ptr = ibu->read.total;
			*op_ptr = IBU_OP_READ;
			*vc_pptr = ibu->vc_ptr;
			ibu->pending_operations--;
			if (ibu->closing && ibu->pending_operations == 0)
			{
			    MPIDI_DBG_PRINTF((60, FCNAME, "closing ibu after iov read completed."));
			    ibu = IBU_INVALID_QP;
			}
			MPIU_DBG_PRINTFX(("exiting ibu_wait 6\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return MPI_SUCCESS;
		    }
#ifdef MPIDI_CH3_CHANNEL_RNDV
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_RTS_IOV_READ_REQUEST)
		    {
			int found;
			MPIU_DBG_PRINTF(("received rts iov_read.\n"));

			mpi_errno = MPIDI_CH3U_Handle_recv_rndv_pkt(recv_vc_ptr,
								    &recv_vc_ptr->ch.recv_active->ch.pkt,
								    &rreq, &found);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ibu read progress unable to handle incoming rts(get) packet");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait v\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			for (i=0; i<recv_vc_ptr->ch.recv_active->dev.rdma_iov_count; i++)
			{
			    rreq->dev.rdma_iov[i].MPID_IOV_BUF = recv_vc_ptr->ch.recv_active->dev.rdma_iov[i].MPID_IOV_BUF;
			    rreq->dev.rdma_iov[i].MPID_IOV_LEN = recv_vc_ptr->ch.recv_active->dev.rdma_iov[i].MPID_IOV_LEN;
			    rreq->ch.remote_iov_mem[i] = recv_vc_ptr->ch.recv_active->ch.remote_iov_mem[i];
			}
			rreq->dev.rdma_iov_count = recv_vc_ptr->ch.recv_active->dev.rdma_iov_count;
			rreq->dev.rdma_request = recv_vc_ptr->ch.recv_active->dev.rdma_request;

			if (found)
			{
			    mpi_errno = MPIDI_CH3U_Post_data_receive(recv_vc_ptr, found, &rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			    mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq);
			    /* --BEGIN ERROR HANDLING-- */
			    if (mpi_errno != MPI_SUCCESS)
			    {
				mpi_errno = MPIR_Err_create_code (mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
				MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
				return mpi_errno;
			    }
			    /* --END ERROR HANDLING-- */
			}

			rreq = recv_vc_ptr->ch.recv_active;

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;

			/* free the request used to receive the rts packet and iov data */
			MPIU_Object_set_ref(rreq, 0);
			MPIDI_CH3_Request_destroy(rreq);
		    }
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_IOV_READ_REQUEST)
		    {
			MPIU_DBG_PRINTF(("received iov_read.\n"));
			rreq = recv_vc_ptr->ch.recv_active;

			/* A new sender's iov has arrived so set the offset back to zero. */
			rreq->ch.req->ch.siov_offset = 0;

			mpi_errno = MPIDI_CH3_iStartRndvTransfer(recv_vc_ptr, rreq->ch.req);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "ibu read progress unable to handle incoming rts(get) iov");
			    MPIU_DBG_PRINTFX(("exiting ibu_wait v\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;

			/* free the request used to receive the iov data */
			MPIU_Object_set_ref(rreq, 0);
			MPIDI_CH3_Request_destroy(rreq);
		    }
		    else if (recv_vc_ptr->ch.recv_active->kind == MPIDI_CH3I_IOV_WRITE_REQUEST)
		    {
			MPIU_DBG_PRINTF(("received iov_write.\n"));

			/* A new receiver's iov has arrived so set the offset back to zero. */
			recv_vc_ptr->ch.recv_active->ch.req->ch.riov_offset = 0;

			mpi_errno = MPIDI_CH3I_rdma_writev(recv_vc_ptr, recv_vc_ptr->ch.recv_active->ch.req);
			/* --BEGIN ERROR HANDLING-- */
			if (mpi_errno != MPI_SUCCESS)
			{
			    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
			    MPIU_DBG_PRINTFX(("exiting ibu_wait w\n"));
			    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			    return mpi_errno;
			}
			/* --END ERROR HANDLING-- */

			/* return from the wait */
			MPID_Request_release(recv_vc_ptr->ch.recv_active);
			recv_vc_ptr->ch.recv_active = NULL;
			recv_vc_ptr->ch.reading_pkt = TRUE;
			*num_bytes_ptr = 0;
			*vc_pptr = recv_vc_ptr;
			*op_ptr = IBU_OP_WAKEUP;
			MPIU_DBG_PRINTFX(("exiting ibu_wait x\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return MPI_SUCCESS;
			break;
		    }
#endif /* MPIDI_CH3_CHANNEL_RNDV */
		    else
		    {
			mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s %d", "invalid request type", recv_vc_ptr->ch.recv_active->kind);
			MPIU_DBG_PRINTFX(("exiting ibu_wait y\n"));
			MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
			return mpi_errno;
		    }
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > ibu->read.bufflen)
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, ibu->read.bufflen);
		    ibu->read.total = ibu->read.bufflen;
#ifdef USE_INLINE_PKT_RECEIVE
		    MPIU_DBG_PRINTF(("c:buffering %d bytes.\n", num_bytes - ibu->read.bufflen));
		    ibui_buffer_unex_read(ibu, mem_ptr_orig,
					  ibu->read.bufflen + pkt_offset,
					  num_bytes - ibu->read.bufflen);
#else
		    ibui_buffer_unex_read(ibu, mem_ptr, ibu->read.bufflen, num_bytes - ibu->read.bufflen);
#endif
		    ibu->read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, num_bytes);
		    ibu->read.total += num_bytes;
		    /* advance the user pointer */
		    ibu->read.buffer = (char*)(ibu->read.buffer) + num_bytes;
		    ibu->read.bufflen -= num_bytes;
		    /* put the receive packet back in the pool */
#ifdef USE_INLINE_PKT_RECEIVE
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
#else
		    ibuBlockFreeIB(ibu->allocator, mem_ptr);
#endif
		    ibui_post_receive(ibu);
		}
		if (ibu->read.bufflen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    *num_bytes_ptr = ibu->read.total;
		    *op_ptr = IBU_OP_READ;
		    *vc_pptr = ibu->vc_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibu after simple read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    MPIU_DBG_PRINTFX(("exiting ibu_wait 7\n"));
		    return MPI_SUCCESS;
		}
	    }
	    break;
	default:
	    if (completion_data.status != VAPI_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: unknown completion status = %s != VAPI_SUCCESS\n", 
		    FCNAME, VAPI_strerror(completion_data.status));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", VAPI_strerror(completion_data.status));
		MPIU_DBG_PRINTF(("at time of error: total_s: %d, total_r: %d, posted_r: %d, posted_s: %d\n", g_num_sent, g_num_received, g_num_posted_receives, g_num_posted_sends));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 42\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return mpi_errno;
	    }
	    MPIU_Internal_error_printf("%s: unknown ib opcode: %s\n", FCNAME, op2str(completion_data.opcode));
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", op2str(completion_data.opcode));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait z\n"));
	    return mpi_errno;
	    break;
	}
    }

    MPIU_DBG_PRINTFX(("exiting ibu_wait 8\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
    return MPI_SUCCESS;
}

#endif /* USE_IB_VAPI */
