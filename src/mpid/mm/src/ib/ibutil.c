/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

int ibu_post_receive(MPIDI_VC *vc_ptr)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_rcv_t work_req;
    void *mem_ptr;

    mem_ptr = BlockAlloc(vc_ptr->data.ib.info.m_allocator);

    sg_list.data_seg_p = &data;
    sg_list.data_seg_num = 1;
    data.length = IB_PACKET_SIZE;
    data.va = (ib_uint64_t)mem_ptr;
    data.l_key = vc_ptr->data.ib.info.m_lkey;
    work_req.op_type = OP_RECEIVE;
    work_req.sg_list = sg_list;
    /* store the VC ptr and the mem ptr in the work id */
    work_req.work_req_id = (ib_uint64_t)vc_ptr;
    work_req.work_req_id = work_req.work_req_id << 32;
    work_req.work_req_id = work_req.work_req_id | (ib_uint64_t)mem_ptr;

    status = ib_post_rcv_req_us(IB_Process.hca_handle, 
				vc_ptr->data.ib.info.m_qp_handle,
				&work_req);
    if (status != IB_SUCCESS)
    {
	err_printf("Error: failed to post ib receive, status = %d\n", status);
    }

    return MPI_SUCCESS;
}
