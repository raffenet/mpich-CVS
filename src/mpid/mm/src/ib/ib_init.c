/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"
#include "pmi.h"

IB_PerProcess IB_Process;
MPIDI_VC_functions g_ib_vc_functions = 
{
    ib_post_read,
    ib_enqueue_read_at_head,
    ib_merge_with_unexpected,
    ib_merge_with_posted,
    ib_merge_unexpected_data,
    ib_post_write,
    ib_enqueue_write_at_head,
    ib_reset_car,
    ib_setup_packet_car,
    ib_post_read_pkt
};

ib_uint32_t modifyQP( IB_Info *ib, Ib_qp_state qp_state )
{
    ib_uint32_t status;
    ib_qp_attr_list_t attrList;
    ib_address_vector_t av;
    attr_rec_t *attr_rec = NULL;

    if (qp_state == IB_QP_STATE_INIT)
    {
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    MPIU_dbg_printf("Malloc failed %d\n", __LINE__);
	    return IB_FAILURE;
	}
	    
	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_PORT;
	((attr_rec[0]).data) = 1;
	((attr_rec[1]).id) = IB_QP_ATTR_PRIMARY_P_KEY_IX;
	((attr_rec[1]).data) = 0;
	((attr_rec[2]).id) = IB_QP_ATTR_RDMA_W_F;
	((attr_rec[2]).data) = 1;
	((attr_rec[3]).id) = IB_QP_ATTR_RDMA_R_F;
	((attr_rec[3]).data) = 1;
	((attr_rec[4]).id) = IB_QP_ATTR_ATOMIC_F;
	((attr_rec[4]).data) = 0;
	    
	attrList.attr_num = 5;
	attrList.attr_rec_p = &attr_rec[0];    
    }
    else if (qp_state == IB_QP_STATE_RTR) 
    {
	av.sl                         = 0;
	/*MPIU_dbg_printf("setting dest_lid to ib->m_dlid: %d\n", ib->m_dlid);*/
	av.dest_lid                   = (ib_uint16_t)ib->m_dlid;
	av.grh_f                      = 0;
	av.path_bits                  = 0;
	av.max_static_rate            = 1;
	av.global.flow_label          = 1;
	av.global.hop_limit           = 1;
	av.global.src_gid_index       = 0;
	av.global.traffic_class       = 1;
	    
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 6)) == NULL )
	{
	    MPIU_dbg_printf("Malloc failed %d\n", __LINE__);
	    return IB_FAILURE;
	}

	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_ADDR;
	((attr_rec[0]).data) = (int)&av;
	((attr_rec[1]).id) = IB_QP_ATTR_DEST_QPN;
	((attr_rec[1]).data) = ib->m_dest_qp_num;
	((attr_rec[2]).id) = IB_QP_ATTR_RCV_PSN;
	((attr_rec[2]).data) = 0;
	((attr_rec[3]).id) = IB_QP_ATTR_MTU;
	((attr_rec[3]).data) = ib->m_mtu_size;
	((attr_rec[4]).id) = IB_QP_ATTR_RDMA_READ_LIMIT;
	((attr_rec[4]).data) = 4;
	((attr_rec[5]).id) = IB_QP_ATTR_RNR_NAK_TIMER;
	((attr_rec[5]).data) = 1;
	    
	attrList.attr_num = 6;
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RTS)
    {
	if ((attr_rec = (attr_rec_t *)
	     malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    MPIU_dbg_printf("Malloc failed %d\n", __LINE__);
	    return IB_FAILURE;
	}

	((attr_rec[0]).id)    = IB_QP_ATTR_SEND_PSN;
	((attr_rec[0]).data)  = 0; 
	((attr_rec[1]).id)    = IB_QP_ATTR_TIMEOUT;
	((attr_rec[1]).data)  = 0x7c;
	((attr_rec[2]).id)    = IB_QP_ATTR_RETRY_COUNT;
	((attr_rec[2]).data)  = 2048;
	((attr_rec[3]).id)    = IB_QP_ATTR_RNR_RETRY_COUNT;
	((attr_rec[3]).data)  = 2048;
	((attr_rec[4]).id)    = IB_QP_ATTR_DEST_RDMA_READ_LIMIT;
	((attr_rec[4]).data)  = 4;
	
	attrList.attr_num = 5; 
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RESET)
    {
	attrList.attr_num = 0;
	attrList.attr_rec_p = NULL;
    }
    else
    {
	return IB_FAILURE;
    }

    status = ib_qp_modify_us(IB_Process.hca_handle, 
			     ib->m_qp_handle, 
			     qp_state, 
			     &attrList );
    if (attr_rec)    
	free(attr_rec);
    if( status != IB_SUCCESS )
    {
	return status;
    }

    return IB_SUCCESS;
}

ib_uint32_t createQP(IB_Info *ib)
{
    ib_uint32_t status;
    ib_uint32_t qp_num;
    ib_qp_attr_list_t attrList;
    attr_rec_t attr_rec[] = {
	{IB_QP_ATTR_SERVICE_TYPE, IB_ST_RELIABLE_CONNECTION},
	{IB_QP_ATTR_SEND_CQ, 0},
	{IB_QP_ATTR_RCV_CQ, 0},
	{IB_QP_ATTR_SEND_REQ_MAX, 0},
	{IB_QP_ATTR_RCV_REQ_MAX, 0},
	{IB_QP_ATTR_SEND_SGE_MAX, 8},
	{IB_QP_ATTR_RCV_SGE_MAX, 8},
	{IB_QP_ATTR_SIGNALING_TYPE, QP_SIGNAL_ALL}
    };

    attr_rec[1].data = (int)ib->m_send_cq_handle;
    attr_rec[2].data = (int)ib->m_recv_cq_handle;
    attr_rec[3].data = ib->m_max_wqes;
    attr_rec[4].data = ib->m_max_wqes;

    attrList.attr_num = sizeof(attr_rec)/sizeof(attr_rec[0]);
    attrList.attr_rec_p = &attr_rec[0];

    status = ib_qp_create_us(
	IB_Process.hca_handle,
	IB_Process.pd_handle,
	&attrList, 
	&ib->m_qp_handle, 
	&qp_num, 
	NULL);
    if (status != IBA_OK)
	return status;
    ib->m_dest_qp_num = qp_num;
    return IB_SUCCESS;
}

int ib_setup_connections()
{
    MPID_Comm *comm_ptr;
    int mpi_errno;
    MPIDI_VC *vc_ptr;
    int i, len;
    char *key, *value;
    IB_Info *ib;
    ib_uint32_t status;
    ib_uint32_t lkey, rkey;
    ib_uint32_t max_cq_entries = IB_MAX_CQ_ENTRIES+1;
    MPIDI_STATE_DECL(MPID_STATE_IB_SETUP_CONNECTIONS);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_SETUP_CONNECTIONS);

    len = PMI_KVS_Get_key_length_max();
    key = (char*)malloc(len * sizeof(char));
    len = PMI_KVS_Get_value_length_max();
    value = (char*)malloc(len * sizeof(char));

    /* setup the vc's on comm_world */
    comm_ptr = MPIR_Process.comm_world;

    /* allocate a vc reference table */
    mpi_errno = MPID_VCRT_Create(comm_ptr->remote_size, &comm_ptr->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_dbg_printf("MPID_VCRT_Create failed, error %d\n", mpi_errno);
	free(key);
	free(value);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	return -1;
    }
    /* get an alias to the array of vc pointers */
    mpi_errno = MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	MPIU_dbg_printf("MPID_VCRT_Get_ptr failed, error %d\n", mpi_errno);
	free(key);
	free(value);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	return -1;
    }

    for (i=0; i<comm_ptr->remote_size; i++)
    {
	if ( i == comm_ptr->rank)
	    continue; /* don't make a connection to myself */
	/*MPIU_dbg_printf("setting up VC connection to rank %d\n", i);*/
	vc_ptr = comm_ptr->vcr[i];
	if (vc_ptr == NULL)
	{
	    comm_ptr->vcr[i] = vc_ptr = mm_vc_alloc(MM_IB_METHOD);
	    /* copy the kvs name and rank into the vc. 
	       this may not be necessary */
	    vc_ptr->pmi_kvsname = comm_ptr->mm.pmi_kvsname;
	    vc_ptr->rank = i;
	}
	sprintf(key, "ib_lid_%d", i);
	PMI_KVS_Get(vc_ptr->pmi_kvsname, key, value);
	/*MPIU_dbg_printf("PMI_KVS_Get %s:<%s, %s>\n", 
			vc_ptr->pmi_kvsname, key, value);*/
	ib = &vc_ptr->data.ib.info;
	ib->m_dlid = atoi(value);
	/*MPIU_dbg_printf("pinning %d bytes of memory\n", IB_PINNED_MEMORY_SIZE);*/
	ib->m_virtual_address = malloc(IB_PINNED_MEMORY_SIZE);
	if (ib->m_virtual_address == NULL)
	{
	    MPIU_dbg_printf("malloc(%d) failed.\n", IB_PINNED_MEMORY_SIZE);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	status = ib_mr_register_us(IB_Process.hca_handle,
				   (ib_uint8_t*)ib->m_virtual_address,
				   IB_PINNED_MEMORY_SIZE,
				   IB_Process.pd_handle,
				   IB_ACCESS_LOCAL_WRITE,
				   &ib->m_mr_handle,
				   &lkey, &rkey);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("ib_mr_register_us failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	ib->m_polling = TRUE;
	ib->m_message_size = IB_PINNED_MEMORY_SIZE;
	ib->m_message_segments = 1;
	ib->m_max_wqes = 50;
	ib->m_mtu_size = 2048;
	/* ***************************************** */
	/* These fields were used by the Paceline code.
	   I suspect I won't need to use them */
	ib->m_rcv_completion_counter = 0;
	ib->m_rcv_work_id = 0;
	ib->m_rcv_posted = 0;
	ib->m_snd_completion_counter = 0;
	ib->m_snd_work_id = 0;
	ib->m_snd_posted = 0;
	/* ***************************************** */
	/*MPIU_dbg_printf("creating the send/recv completion queues\n");*/
	max_cq_entries = IB_MAX_CQ_ENTRIES + 1;
	status = ib_cq_create_us(IB_Process.hca_handle, 
				 IB_Process.cqd_handle,
				 &max_cq_entries,
				 &ib->m_send_cq_handle,
				 NULL);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("ib_cq_create_us(send cq) failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	max_cq_entries = IB_MAX_CQ_ENTRIES + 1;
	status = ib_cq_create_us(IB_Process.hca_handle, 
				 IB_Process.cqd_handle,
				 &max_cq_entries,
				 &ib->m_recv_cq_handle,
				 NULL);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("ib_cq_create_us(recv cq) failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}

	/*******************************************
	  Is this section on setting up receive and 
	  send segments bogus? Does it assume there
	  will only be one posted send or recv?
	********************************************/

	/*MPIU_dbg_printf("allocating the descriptor segments\n");*/

	/* allocate and setup the receive segments */
	ib->m_recv_sglist.data_seg_p = calloc(ib->m_message_segments,
					      sizeof(ib_data_segment_t));
	if (ib->m_recv_sglist.data_seg_p == NULL)
	{
	    MPIU_dbg_printf("calloc failed\n");
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	ib->m_recv_sglist.data_seg_num = ib->m_message_segments;
	for (i=0; i<(int)ib->m_message_segments; i++)
	{
	    ib->m_recv_sglist.data_seg_p[i].length = ib->m_message_size;
	    ib->m_recv_sglist.data_seg_p[i].va = (ib_uint64_t)ib->m_virtual_address;
	    ib->m_recv_sglist.data_seg_p[i].l_key = lkey;
	}

	/* allocate and setup the send segments */
	ib->m_send_sglist.data_seg_p = calloc(ib->m_message_segments,
					      sizeof(ib_data_segment_t));
	if (ib->m_send_sglist.data_seg_p == NULL)
	{
	    MPIU_dbg_printf("calloc failed\n");
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	ib->m_send_sglist.data_seg_num = ib->m_message_segments;
	for (i=0; i<(int)ib->m_message_segments; i++)
	{
	    ib->m_send_sglist.data_seg_p[i].length = ib->m_message_size;
	    ib->m_send_sglist.data_seg_p[i].va = (ib_uint64_t)ib->m_virtual_address;
	    ib->m_send_sglist.data_seg_p[i].l_key = lkey;
	}

	/*MPIU_dbg_printf("creating the queue pair\n");*/
	/* Create the queue pair */
	status = createQP(ib);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("createQP failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}

	/*MPIU_dbg_printf("modifyQP(INIT)\n");*/
	status = modifyQP(ib, IB_QP_STATE_INIT);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("modifyQP(INIT) failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	/*MPIU_dbg_printf("modifyQP(RTR)\n");*/
	status = modifyQP(ib, IB_QP_STATE_RTR);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("modifyQP(RTR) failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
	/*MPIU_dbg_printf("modifyQP(RTS)\n");*/
	status = modifyQP(ib, IB_QP_STATE_RTS);
	if (status != IB_SUCCESS)
	{
	    MPIU_dbg_printf("modifyQP(RTS) failed, error %d\n", status);
	    free(key);
	    free(value);
	    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
	    return -1;
	}
    }

    /*MPIU_dbg_printf("calling PMI_Barrier\n");*/
    PMI_Barrier();
    /*MPIU_dbg_printf("PMI_Barrier returned\n");*/

    free(key);
    free(value);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_SETUP_CONNECTIONS);
    return MPI_SUCCESS;
}

/*@
   ib_init - initialize the ib method

   Notes:
   Let's assume there is one HCA per node with one port available.
@*/
int ib_init()
{
    ib_uint32_t status;
    char key[100], value[100];
    MPIDI_STATE_DECL(MPID_STATE_IB_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_INIT);

    ib_init_us();

    /* Initialize globals */
    /* get a handle to the host channel adapter */
    status = ib_hca_open_us("TORRENT" , &IB_Process.hca_handle);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_hca_open_us failed, status %d\n", status);
	return status;
    }
    /* get a protection domain handle */
    status = ib_pd_allocate_us(IB_Process.hca_handle, &IB_Process.pd_handle);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_pd_allocate_us failed, status %d\n", status);
	return status;
    }
    /* get a completion queue domain handle */
    status = ib_cqd_create_us(IB_Process.hca_handle, &IB_Process.cqd_handle);
#if 0 /* for some reason this function fails when it really is ok */
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_cqd_create_us failed, status %d\n", status);
	return status;
    }
#endif
    /* get the lid */
    status = ib_hca_query_us(IB_Process.hca_handle, &IB_Process.attr, 
			     HCA_QUERY_HCA_STATIC);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_hca_query_us(HCA_QUERY_HCA_STATIC) failed, status %d\n", status);
	return status;
    }
    IB_Process.attr.port_dynamic_info_p = 
	(port_dynamic_info_t*)malloc(IB_Process.attr.node_info.port_num * 
				     sizeof(port_dynamic_info_t));
    status = ib_hca_query_us(IB_Process.hca_handle, &IB_Process.attr, 
			     HCA_QUERY_PORT_INFO_DYNAMIC);
    if (status != IB_SUCCESS)
    {
	err_printf("ib_init: ib_hca_query_us(HCA_QUERY_PORT_INFO_DYNAMIC) failed, status %d\n", status);
	return status;
    }
    IB_Process.lid = IB_Process.attr.port_dynamic_info_p->lid;
    // free this structure because the information is transient?
    free(IB_Process.attr.port_dynamic_info_p);
    IB_Process.attr.port_dynamic_info_p = NULL;

    sprintf(key, "ib_lid_%d", MPIR_Process.comm_world->rank);
    sprintf(value, "%d", IB_Process.lid);
    /*MPIU_dbg_printf("ib lid %d\n", IB_Process.lid);*/
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
    PMI_Barrier();

    ib_setup_connections();
    MPIU_dbg_printf("ib_setup_connections returned\n");

    MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
    return MPI_SUCCESS;
}

/*@
   ib_finalize - finalize the ib method

   Notes:
@*/
int ib_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_FINALIZE);

    ib_release_us();

    MPIDI_FUNC_EXIT(MPID_STATE_IB_FINALIZE);
    return MPI_SUCCESS;
}
