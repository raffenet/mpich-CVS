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

#ifdef USE_IB_IBAL

#include "ibuimpl.ibal.h"

/*#define PRINT_IBU_WAIT*/
#ifdef PRINT_IBU_WAIT
#define MPIU_DBG_PRINTFX(a) MPIU_DBG_PRINTF(a)
#else
#define MPIU_DBG_PRINTFX(a)
#endif

char * op2str(int wc_type)
{
    static char str[20];
    switch(wc_type)
    {
    case IB_WC_SEND:
	return "IB_WC_SEND";
    case IB_WC_RDMA_WRITE:
	return "IB_WC_RDMA_WRITE";
    case IB_WC_RECV:
	return "IB_WC_RECV";
    case IB_WC_RDMA_READ:
	return "IB_WC_RDMA_READ";
    case IB_WC_MW_BIND:
	return "IB_WC_MW_BIND";
    case IB_WC_FETCH_ADD:
	return "IB_WC_FETCH_ADD";
    case IB_WC_COMPARE_SWAP:
	return "IB_WC_COMPARE_SWAP";
    case IB_WC_RECV_RDMA_WRITE:
	return "IB_WC_RECV_RDMA_WRITE";
    }
    sprintf(str, "%d", wc_type);
    return str;
}

/* Port me to ibal
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
*/

#undef FUNCNAME
#define FUNCNAME ibu_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_wait(ibu_set_t set, int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr)
{
    int i;
    ib_api_status_t status;
    ib_wc_t *p_in, *p_complete;
    ib_wc_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    ibu_t ibu;
    int num_bytes;
    unsigned int offset;
#ifndef HAVE_32BIT_POINTERS
    ibu_work_id_handle_t *id_ptr;
#endif
    int num_cq_entries;
#ifdef TRACE_IBU
    static int total_num_completions = 0;
#endif
    MPIDI_VC_t *recv_vc_ptr;
    void *mem_ptr_orig;
    int pkt_offset;
    MPIDI_STATE_DECL(MPID_STATE_IBU_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WAIT);
    MPIU_DBG_PRINTFX(("entering ibu_wait\n"));
    for (;;) 
    {
	if (IBU_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes", IBU_Process.unex_finished_list->read.total));
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
	    return IBU_SUCCESS;
	}

	p_complete = NULL;
	completion_data.p_next = NULL;
	p_in = &completion_data;

	/*
	status = ib_rearm_cq(set, FALSE);
	if (status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: ib_rearm_cq failed, %s\n", FCNAME, ib_get_err_str(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait -1\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

	status = ib_poll_cq(set, &p_in, &p_complete); 
	if (status == IB_NOT_FOUND && p_complete == NULL)
	{
	    /* ibu_wait polls until there is something in the queue */
	    /* or the timeout has expired */
	    if (millisecond_timeout == 0)
	    {
		*num_bytes_ptr = 0;
		*vc_pptr = NULL;
		*op_ptr = IBU_OP_TIMEOUT;
		MPIU_DBG_PRINTFX(("exiting ibu_wait 2\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_SUCCESS;
	    }
	    continue;
	}
	if (status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: error: ib_poll_cq did not return IB_SUCCESS, %s\n", FCNAME, ib_get_err_str(status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 3\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	/*
	if (completion_data.status != IB_SUCCESS)
	{
	    MPIU_Internal_error_printf("%s: completion status = %s != IB_SUCCESS\n", 
		FCNAME, ib_get_err_str(completion_data.status));
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	*/

	/*printf("ib_poll_cq returned %d\n", completion_data.wc_type);fflush(stdout);*/

#ifdef HAVE_32BIT_POINTERS
	ibu = (ibu_t)(((ibu_work_id_handle_t*)&completion_data.wr_id)->data.ptr);
	mem_ptr = (void*)(((ibu_work_id_handle_t*)&completion_data.wr_id)->data.mem);
#else
	id_ptr = *((ibu_work_id_handle_t**)&completion_data.wr_id);
	ibu = (ibu_t)(id_ptr->ptr);
	mem_ptr = (void*)(id_ptr->mem);
	ibuBlockFree(g_workAllocator, (void*)id_ptr);
#endif
	mem_ptr_orig = mem_ptr;

	switch (completion_data.wc_type)
	{
	case IB_WC_SEND:
	    if (completion_data.status != IB_SUCCESS)
	    {
		num_cq_entries = 0;
		status = ib_query_cq(set, &num_cq_entries);
#ifdef TRACE_IBU
		MPIU_Internal_error_printf("%s: send completion status = %d = %s != IB_SUCCESS, cq size = %d, total completions = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries, total_num_completions);
		MPIU_Internal_error_printf("oustanding recvs: %d, total_recvs %d\noutstanding sends: %d, total_sends: %d\n", IBU_Process.outstanding_recvs, IBU_Process.total_recvs, IBU_Process.outstanding_sends, IBU_Process.total_sends);
#else
		MPIU_Internal_error_printf("%s: send completion status = %d = %s != IB_SUCCESS, cq size = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries);
#endif
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
#ifdef TRACE_IBU
	    total_num_completions++;
	    IBU_Process.outstanding_sends--;
	    IBU_Process.total_sends++;
#endif
	    if (mem_ptr == (void*)-1)
	    {
		/* flow control ack completed, no user data so break out here */
		break;
	    }
	    g_cur_write_stack_index--;
	    num_bytes = g_num_bytes_written_stack[g_cur_write_stack_index].length;
	    MPIDI_DBG_PRINTF((60, FCNAME, "send num_bytes = %d\n", num_bytes));
	    if (num_bytes < 0)
	    {
		i = num_bytes;
		num_bytes = 0;
		for (; i<0; i++)
		{
		    g_cur_write_stack_index--;
		    MPIDI_DBG_PRINTF((60, FCNAME, "num_bytes += %d\n", g_num_bytes_written_stack[g_cur_write_stack_index].length));
		    num_bytes += g_num_bytes_written_stack[g_cur_write_stack_index].length;
		    if (g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr == NULL)
			MPIU_Internal_error_printf("ibu_wait: write stack has NULL mem_ptr at location %d\n", g_cur_write_stack_index);
		    MPIU_Assert(g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr != NULL);
		    ibuBlockFreeIB(ibu->allocator, g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr);
		}
	    }
	    else
	    {
		if (mem_ptr == NULL)
		    MPIU_Internal_error_printf("ibu_wait: send mem_ptr == NULL\n");
		MPIU_Assert(mem_ptr != NULL);
		ibuBlockFreeIB(ibu->allocator, mem_ptr);
	    }

	    *num_bytes_ptr = num_bytes;
	    *op_ptr = IBU_OP_WRITE;
	    *vc_pptr = ibu->vc_ptr;
	    MPIU_DBG_PRINTFX(("exiting ibu_wait 5\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_SUCCESS;
	    break;
	case IB_WC_RECV:
	    if (completion_data.status != IB_SUCCESS)
	    {
		num_cq_entries = 0;
		status = ib_query_cq(set, &num_cq_entries);
#ifdef TRACE_IBU
		MPIU_Internal_error_printf("%s: recv completion status = %d = %s != IB_SUCCESS, cq size = %d, total completions = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries, total_num_completions);
		MPIU_Internal_error_printf("oustanding recvs: %d, total_recvs %d\noutstanding sends: %d, total_sends: %d\n", IBU_Process.outstanding_recvs, IBU_Process.total_recvs, IBU_Process.outstanding_sends, IBU_Process.total_sends);
#else
		MPIU_Internal_error_printf("%s: recv completion status = %d = %s != IB_SUCCESS, cq size = %d\n", 
					   FCNAME, completion_data.status, ib_get_err_str(completion_data.status), num_cq_entries);
#endif
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
#ifdef TRACE_IBU
	    total_num_completions++;
	    IBU_Process.outstanding_recvs--;
	    IBU_Process.total_recvs++;
#endif
	    if (completion_data.recv.conn.recv_opt & IB_RECV_OPT_IMMEDIATE)
	    {
		ibu->nAvailRemote += completion_data.recv.conn.immediate_data;
		MPIDI_DBG_PRINTF((60, FCNAME, "%d packets acked, nAvailRemote now = %d", completion_data.recv.conn.immediate_data, ibu->nAvailRemote));
		ibuBlockFreeIB(ibu->allocator, mem_ptr);
		ibui_post_receive_unacked(ibu);
		MPIU_Assert(completion_data.length == 1); /* check this after the printfs to see if the immediate data is correct */
		break;
	    }
	    num_bytes = completion_data.length;
	    recv_vc_ptr = (MPIDI_VC *)(ibu->vc_ptr);
	    pkt_offset = 0;
	    if (recv_vc_ptr->ch.reading_pkt)
	    {
		/*mpi_errno =*/ MPIDI_CH3U_Handle_recv_pkt(recv_vc_ptr, (MPIDI_CH3_Pkt_t*)mem_ptr, &recv_vc_ptr->ch.recv_active);
		/*
		if (mpi_errno != MPI_SUCCESS)
		{
		    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %s", "infiniband read progress unable to handle incoming packet");
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return IBU_SUCCESS;
		}
		*/
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    MPIU_DBG_PRINTF(("packet with no data handled.\n"));
		    recv_vc_ptr->ch.reading_pkt = TRUE;
		}
		else
		{
		    /*mpi_errno =*/ ibu_post_readv(ibu, recv_vc_ptr->ch.recv_active->dev.iov, recv_vc_ptr->ch.recv_active->dev.iov_count);
		}
		mem_ptr = (unsigned char *)mem_ptr + sizeof(MPIDI_CH3_Pkt_t);
		num_bytes -= sizeof(MPIDI_CH3_Pkt_t);

		if (num_bytes == 0)
		{
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
		    ibui_post_receive(ibu);
		    continue;
		}
		if (recv_vc_ptr->ch.recv_active == NULL)
		{
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, sizeof(MPIDI_CH3_Pkt_t), num_bytes);
		    continue;
		}
		pkt_offset = sizeof(MPIDI_CH3_Pkt_t);
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "ibu_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(ibu->state & IBU_READING))
	    {
		ibui_buffer_unex_read(ibu, mem_ptr_orig, pkt_offset, num_bytes);
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
		offset = (unsigned char*)iter_ptr - (unsigned char*)mem_ptr;
		ibu->read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the receive packet back in the pool */
		    if (mem_ptr == NULL)
			MPIU_Internal_error_printf("ibu_wait: read mem_ptr == NULL\n");
		    MPIU_Assert(mem_ptr != NULL);
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
		    ibui_post_receive(ibu);
		}
		else
		{
		    /* save the unused but received data */
		    ibui_buffer_unex_read(ibu, mem_ptr_orig, offset + pkt_offset, num_bytes);
		}
		if (ibu->read.iovlen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    *num_bytes_ptr = ibu->read.total;
		    *op_ptr = IBU_OP_READ;
		    *vc_pptr = ibu->vc_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibuet after iov read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIU_DBG_PRINTFX(("exiting ibu_wait 6\n"));
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return IBU_SUCCESS;
		}
	    }
	    else
	    {
		if ((unsigned int)num_bytes > ibu->read.bufflen)
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, ibu->read.bufflen);
		    ibu->read.total = ibu->read.bufflen;
		    ibui_buffer_unex_read(ibu, mem_ptr_orig,
					  ibu->read.bufflen + pkt_offset,
					  num_bytes - ibu->read.bufflen);
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
		    ibuBlockFreeIB(ibu->allocator, mem_ptr_orig);
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
		    return IBU_SUCCESS;
		}
	    }
	    break;
	default:
	    if (completion_data.status != IB_SUCCESS)
	    {
		MPIU_Internal_error_printf("%s: unknown completion status = %s != IB_SUCCESS\n", 
					   FCNAME, ib_get_err_str(completion_data.status));
		MPIU_DBG_PRINTFX(("exiting ibu_wait 4\n"));
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_FAIL;
	    }
	    MPIU_Internal_error_printf("%s: unknown ib wc_type: %s\n", FCNAME, op2str(completion_data.wc_type));
	    return IBU_FAIL;
	    break;
	}
    }

    MPIU_DBG_PRINTFX(("exiting ibu_wait 8\n"));
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
    return MPI_SUCCESS;
}

#endif /* USE_IB_IBAL */
