/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

int ib_handle_accept()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_ACCEPT);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_ACCEPT);
    return MPI_SUCCESS;
}

/*@
   ib_make_progress - make progress

   Notes:
@*/
int ib_make_progress()
{
    ib_uint32_t status;
    ib_work_completion_t completion_data;
    MPIDI_VC *vc_ptr;
    void *mem_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IB_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MAKE_PROGRESS);

    status = ib_completion_poll_us(
	IB_Process.hca_handle,
	IB_Process.cq_handle,
	&completion_data);
    if (status == IBA_CQ_EMPTY)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
	return MPI_SUCCESS;
    }
    if (status != IBA_OK)
    {
	err_printf("error: ib_completion_poll_us did not return IBA_OK\n");
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
	return -1;
    }
    if (completion_data.status != IB_COMP_ST_SUCCESS)
    {
	err_printf("error: status = %d != IB_COMP_ST_SUCCESS\n", 
	    completion_data.status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
	return -1;
    }

    /* Get the vc_ptr and mem_ptr out of the work_id */
    vc_ptr = (MPIDI_VC*)(((ib_work_id_handle_t*)&completion_data.work_req_id)->data.vc);
    mem_ptr = (void*)(((ib_work_id_handle_t*)&completion_data.work_req_id)->data.mem);

    switch (completion_data.op_type)
    {
    case OP_SEND:
	ib_handle_written(vc_ptr, mem_ptr, completion_data.bytes_num);
	break;
    case OP_RECEIVE:
	ib_handle_read(vc_ptr, mem_ptr, completion_data.bytes_num);
	break;
    default:
	MPIU_dbg_printf("unknown ib op_type: %d\n", completion_data.op_type);
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
    return MPI_SUCCESS;
}

#endif
