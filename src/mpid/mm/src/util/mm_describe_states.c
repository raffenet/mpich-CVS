#include "mpidimpl.h"

/* This section of code is for the DLOG logging library */
#if (USE_LOGGING == MPID_LOGGING_DLOG)

int MPIDU_Describe_timer_states()
{
    /* mpid functions */
    g_timer_state[MPID_STATE_MPID_ISEND].name = "MPID_Isend";
    g_timer_state[MPID_STATE_MPID_IRECV].name = "MPID_Irecv";
    g_timer_state[MPID_STATE_MPID_SEND].name = "MPID_Send";
    g_timer_state[MPID_STATE_MPID_RECV].name = "MPID_Recv";
    g_timer_state[MPID_STATE_MPID_PROGRESS_TEST].name = "MPID_Progress_test";
    g_timer_state[MPID_STATE_MPID_ABORT].name = "MPID_Abort";
    g_timer_state[MPID_STATE_MPID_CLOSE_PORT].name = "MPID_Close_port";
    g_timer_state[MPID_STATE_MPID_COMM_ACCEPT].name = "MPID_Comm_accept";
    g_timer_state[MPID_STATE_MPID_COMM_CONNECT].name = "MPID_Comm_connect";
    g_timer_state[MPID_STATE_MPID_COMM_DISCONNECT].name = "MPID_Comm_disconnect";
    g_timer_state[MPID_STATE_MPID_COMM_SPAWN_MULTIPLE].name = "MPID_Comm_spawn_multiple";
    g_timer_state[MPID_STATE_MPID_OPEN_PORT].name = "MPID_Open_port";
    g_timer_state[MPID_STATE_MPID_PROGRESS_WAIT].name = "MPID_Progress_wait";
    g_timer_state[MPID_STATE_MPID_REQUEST_RELEASE].name = "MPID_Request_release";

    /* util functions */
    g_timer_state[MPID_STATE_BREAD].name = "bread";
    g_timer_state[MPID_STATE_BREADV].name = "breadv";
    g_timer_state[MPID_STATE_BWRITE].name = "bwrite";
    g_timer_state[MPID_STATE_BWRITEV].name = "bwritev";
    g_timer_state[MPID_STATE_BSELECT].name = "bselect";

    /* mm functions */
    g_timer_state[MPID_STATE_MM_OPEN_PORT].name = "mm_open_port";
    g_timer_state[MPID_STATE_MM_CLOSE_PORT].name = "mm_close_port";
    g_timer_state[MPID_STATE_MM_ACCEPT].name = "mm_accept";
    g_timer_state[MPID_STATE_MM_CONNECT].name = "mm_connect";
    g_timer_state[MPID_STATE_MM_SEND].name = "mm_send";
    g_timer_state[MPID_STATE_MM_RECV].name = "mm_recv";
    g_timer_state[MPID_STATE_MM_CLOSE].name = "mm_close";
    g_timer_state[MPID_STATE_MM_REQUEST_ALLOC].name = "mm_request_alloc";
    g_timer_state[MPID_STATE_MM_REQUEST_FREE].name = "mm_request_free";
    g_timer_state[MPID_STATE_MM_CAR_INIT].name = "mm_car_init";
    g_timer_state[MPID_STATE_MM_CAR_FINALIZE].name = "mm_car_finalize";
    g_timer_state[MPID_STATE_MM_CAR_ALLOC].name = "mm_car_alloc";
    g_timer_state[MPID_STATE_MM_CAR_FREE].name = "mm_car_free";
    g_timer_state[MPID_STATE_MM_VC_INIT].name = "mm_vc_init";
    g_timer_state[MPID_STATE_MM_VC_FINALIZE].name = "mm_vc_finalize";
    g_timer_state[MPID_STATE_MM_VC_FROM_COMMUNICATOR].name = "mm_vc_from_communicator";
    g_timer_state[MPID_STATE_MM_VC_FROM_CONTEXT].name = "mm_vc_from_context";
    g_timer_state[MPID_STATE_MM_VC_ALLOC].name = "mm_vc_alloc";
    g_timer_state[MPID_STATE_MM_VC_CONNECT_ALLOC].name = "mm_vc_connect_alloc";
    g_timer_state[MPID_STATE_MM_VC_FREE].name = "mm_vc_free";
    g_timer_state[MPID_STATE_MM_CHOOSE_BUFFER].name = "mm_choose_buffer";
    g_timer_state[MPID_STATE_MM_RESET_CARS].name = "mm_reset_cars";
    g_timer_state[MPID_STATE_MM_GET_BUFFERS_TMP].name = "mm_get_buffers_tmp";
    g_timer_state[MPID_STATE_MM_RELEASE_BUFFERS_TMP].name = "mm_release_buffers_tmp";
    g_timer_state[MPID_STATE_MM_GET_BUFFERS_VEC].name = "mm_get_buffers_vec";
    g_timer_state[MPID_STATE_VEC_BUFFER_INIT].name = "vec_buffer_init";
    g_timer_state[MPID_STATE_TMP_BUFFER_INIT].name = "tmp_buffer_init";
    g_timer_state[MPID_STATE_SIMPLE_BUFFER_INIT].name = "simple_buffer_init";
    g_timer_state[MPID_STATE_MM_POST_RECV].name = "mm_post_recv";
    g_timer_state[MPID_STATE_MM_POST_SEND].name = "mm_post_send";
    g_timer_state[MPID_STATE_MM_POST_RNDV_DATA_SEND].name = "mm_post_rndv_data_send";
    g_timer_state[MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND].name = "mm_post_rndv_clear_to_send";
    g_timer_state[MPID_STATE_MM_CQ_TEST].name = "mm_cq_test";
    g_timer_state[MPID_STATE_MM_CQ_WAIT].name = "mm_cq_wait";
    g_timer_state[MPID_STATE_MM_CQ_ENQUEUE].name = "mm_cq_enqueue";
    g_timer_state[MPID_STATE_MM_CREATE_POST_UNEX].name = "mm_create_post_unex";
    g_timer_state[MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND].name = "mm_enqueue_request_to_send";
    g_timer_state[MPID_STATE_CQ_HANDLE_READ_HEAD_CAR].name = "cq_handle_read_head_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_READ_DATA_CAR].name = "cq_handle_read_data_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_READ_CAR].name = "cq_handle_read_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_WRITE_HEAD_CAR].name = "cq_handle_write_head_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_WRITE_DATA_CAR].name = "cq_handle_write_data_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_WRITE_CAR].name = "cq_handle_write_car";

    /* xfer functions */
    g_timer_state[MPID_STATE_XFER_INIT].name = "xfer_init";
    g_timer_state[MPID_STATE_XFER_RECV_OP].name = "xfer_recv_op";
    g_timer_state[MPID_STATE_XFER_RECV_MOP_OP].name = "xfer_recv_mop_op";
    g_timer_state[MPID_STATE_XFER_RECV_FORWARD_OP].name = "xfer_recv_forward_op";
    g_timer_state[MPID_STATE_XFER_RECV_MOP_FORWARD_OP].name = "xfer_mop_forward_op";
    g_timer_state[MPID_STATE_XFER_FORWARD_OP].name = "xfer_forward_op";
    g_timer_state[MPID_STATE_XFER_SEND_OP].name = "xfer_send_op";
    g_timer_state[MPID_STATE_XFER_REPLICATE_OP].name = "xfer_replicate_op";
    g_timer_state[MPID_STATE_XFER_START].name = "xfer_start";

    /* method functions */
    g_timer_state[MPID_STATE_TCP_INIT].name = "tcp_init";
    g_timer_state[MPID_STATE_TCP_FINALIZE].name = "tcp_finalize";
    g_timer_state[MPID_STATE_TCP_ACCEPT_CONNECTION].name = "tcp_accept_connection";
    g_timer_state[MPID_STATE_TCP_GET_BUSINESS_CARD].name = "tcp_get_business_card";
    g_timer_state[MPID_STATE_TCP_CAN_CONNECT].name = "tcp_can_connect";
    g_timer_state[MPID_STATE_TCP_POST_CONNECT].name = "tcp_post_connect";
    g_timer_state[MPID_STATE_TCP_POST_READ].name = "tcp_post_read";
    g_timer_state[MPID_STATE_TCP_MERGE_WITH_UNEXPECTED].name = "tcp_merge_with_unexpected";
    g_timer_state[MPID_STATE_TCP_POST_WRITE].name = "tcp_post_write";
    g_timer_state[MPID_STATE_TCP_MAKE_PROGRESS].name = "tcp_make_progress";
    g_timer_state[MPID_STATE_TCP_CAR_ENQUEUE].name = "tcp_car_enqueue";
    g_timer_state[MPID_STATE_TCP_CAR_DEQUEUE].name = "tcp_car_dequeue";
    g_timer_state[MPID_STATE_TCP_CAR_DEQUEUE_WRITE].name = "tcp_dequeue_write";
    g_timer_state[MPID_STATE_TCP_RESET_CAR].name = "tcp_reset_car";
    g_timer_state[MPID_STATE_TCP_POST_READ_PKT].name = "tcp_post_read_pkt";
    g_timer_state[MPID_STATE_TCP_READ].name = "tcp_read";
    g_timer_state[MPID_STATE_TCP_WRITE].name = "tcp_write";
    g_timer_state[MPID_STATE_TCP_READ_SHM].name = "tcp_read_shm";
    g_timer_state[MPID_STATE_TCP_READ_VIA].name = "tcp_read_via";
    g_timer_state[MPID_STATE_TCP_READ_VIA_RDMA].name = "tcp_read_via_rdma";
    g_timer_state[MPID_STATE_TCP_READ_VEC].name = "tcp_read_vec";
    g_timer_state[MPID_STATE_TCP_READ_TMP].name = "tcp_read_tmp";
    g_timer_state[MPID_STATE_TCP_READ_CONNECTING].name = "tcp_read_connecting";
    g_timer_state[MPID_STATE_TCP_WRITE_SHM].name = "tcp_write_shm";
    g_timer_state[MPID_STATE_TCP_WRITE_VIA].name = "tcp_write_via";
    g_timer_state[MPID_STATE_TCP_WRITE_VIA_RDMA].name = "tcp_write_via_rdma";
    g_timer_state[MPID_STATE_TCP_WRITE_VEC].name = "tcp_write_vec";
    g_timer_state[MPID_STATE_TCP_WRITE_TMP].name = "tcp_write_tmp";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_SHM].name = "tcp_stuff_vector_shm";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_VIA].name = "tcp_stuff_vector_via";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_VIA_RDMA].name = "tcp_stuff_vector_via_rdma";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_VEC].name = "tcp_stuff_vector_vec";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_TMP].name = "tcp_stuff_vector_tmp";
    g_timer_state[MPID_STATE_TCP_WRITE_AGGRESSIVE].name = "tcp_write_aggressive";
    g_timer_state[MPID_STATE_TCP_CAR_HEAD_ENQUEUE].name = "tcp_car_head_enqueue";
    g_timer_state[MPID_STATE_TCP_SETUP_PACKET_CAR].name = "tcp_setup_packet_car";
    g_timer_state[MPID_STATE_TCP_UPDATE_CAR_NUM_WRITTEN].name = "tcp_update_car_num_written";
    g_timer_state[MPID_STATE_TCP_MERGE_UNEXPECTED_DATA].name = "tcp_merge_unexpected_data";
    g_timer_state[MPID_STATE_TCP_MERGE_SHM].name = "tcp_merge_shm";
    g_timer_state[MPID_STATE_TCP_MERGE_VIA].name = "tcp_merge_via";
    g_timer_state[MPID_STATE_TCP_MERGE_VIA_RDMA].name = "tcp_merge_via_rdma";
    g_timer_state[MPID_STATE_TCP_MERGE_VEC].name = "tcp_merge_vec";
    g_timer_state[MPID_STATE_TCP_MERGE_TMP].name = "tcp_merge_tmp";
    g_timer_state[MPID_STATE_TCP_MERGE_SIMPLE].name = "tcp_merge_simple";
    g_timer_state[MPID_STATE_TCP_MERGE_WITH_POSTED].name = "tcp_merge_with_posted";
    g_timer_state[MPID_STATE_TCP_READ_HEADER].name = "tcp_read_header";
    g_timer_state[MPID_STATE_TCP_READ_DATA].name = "tcp_read_data";
    g_timer_state[MPID_STATE_TCP_READ_SIMPLE].name = "tcp_read_simple";
    g_timer_state[MPID_STATE_TCP_WRITE_SIMPLE].name = "tcp_write_simple";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_SIMPLE].name = "tcp_stuff_vector_simple";
    g_timer_state[MPID_STATE_FIND_IN_QUEUE].name = "find_in_queue";

    g_timer_state[MPID_STATE_SHM_CAN_CONNECT].name = "shm_can_connect";
    g_timer_state[MPID_STATE_SHM_GET_BUSINESS_CARD].name = "shm_get_business_card";
    g_timer_state[MPID_STATE_SHM_INIT].name = "shm_init";
    g_timer_state[MPID_STATE_SHM_FINALIZE].name = "shm_finalize";
    g_timer_state[MPID_STATE_SHM_MAKE_PROGRESS].name = "shm_make_progress";
    g_timer_state[MPID_STATE_SHM_ALLOC].name = "shm_alloc";
    g_timer_state[MPID_STATE_SHM_FREE].name = "shm_free";
    g_timer_state[MPID_STATE_SHM_GET_MEM_SYNC].name = "shm_get_mem_sync";
    g_timer_state[MPID_STATE_SHM_RELEASE_MEM].name = "shm_release_mem";
    g_timer_state[MPID_STATE_SHM_POST_CONNECT].name = "shm_post_connect";
    g_timer_state[MPID_STATE_SHM_POST_READ].name = "shm_post_read";
    g_timer_state[MPID_STATE_SHM_MERGE_WITH_UNEXPECTED].name = "shm_merge_with_unexpected";
    g_timer_state[MPID_STATE_SHM_POST_WRITE].name = "shm_post_write";

    g_timer_state[MPID_STATE_PACKER_CAR_ENQUEUE].name = "packer_car_enqueue";
    g_timer_state[MPID_STATE_PACKER_CAR_DEQUEUE].name = "packer_car_dequeue";
    g_timer_state[MPID_STATE_PACKER_INIT].name = "packer_init";
    g_timer_state[MPID_STATE_PACKER_FINALIZE].name = "packer_finalize";
    g_timer_state[MPID_STATE_PACKER_MAKE_PROGRESS].name = "packer_make_progress";
    g_timer_state[MPID_STATE_PACKER_POST_READ].name = "packer_post_read";
    g_timer_state[MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED].name = "packer_merge_with_unexpected";
    g_timer_state[MPID_STATE_PACKER_POST_WRITE].name = "packer_post_write";
    g_timer_state[MPID_STATE_PACKER_RESET_CAR].name = "packer_reset_car";

    g_timer_state[MPID_STATE_UNPACKER_CAR_ENQUEUE].name = "unpacker_car_enqueue";
    g_timer_state[MPID_STATE_UNPACKER_CAR_DEQUEUE].name = "unpacker_car_dequeue";
    g_timer_state[MPID_STATE_UNPACKER_INIT].name = "unpacker_init";
    g_timer_state[MPID_STATE_UNPACKER_FINALIZE].name = "unpacker_finalize";
    g_timer_state[MPID_STATE_UNPACKER_MAKE_PROGRESS].name = "unpacker_make_progress";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_SHM].name = "unpacker_write_shm";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_VIA].name = "unpacker_write_via";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_VIA_RDMA].name = "unpacker_write_via_rdma";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_VEC].name = "unpacker_write_vec";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_TMP].name = "unpacker_write_tmp";
    g_timer_state[MPID_STATE_UNPACKER_POST_READ].name = "unpacker_post_read";
    g_timer_state[MPID_STATE_UNPACKER_MERGE_WITH_UNEXPECTED].name = "unpacker_merge_with_unexpected";
    g_timer_state[MPID_STATE_UNPACKER_POST_WRITE].name = "unpacker_post_write";
    g_timer_state[MPID_STATE_UNPACKER_RESET_CAR].name = "unpacker_reset_car";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_SIMPLE].name = "unpacker_write_simple";

    g_timer_state[MPID_STATE_VIA_CAN_CONNECT].name = "via_can_connect";
    g_timer_state[MPID_STATE_VIA_GET_BUSINESS_CARD].name = "via_get_business_card";
    g_timer_state[MPID_STATE_VIA_INIT].name = "via_init";
    g_timer_state[MPID_STATE_VIA_FINALIZE].name = "via_finalize";
    g_timer_state[MPID_STATE_VIA_MAKE_PROGRESS].name = "via_make_progress";
    g_timer_state[MPID_STATE_VIA_POST_CONNECT].name = "via_post_connect";
    g_timer_state[MPID_STATE_VIA_POST_READ].name = "via_post_read";
    g_timer_state[MPID_STATE_VIA_MERGE_WITH_UNEXPECTED].name = "via_merge_with_unexpected";
    g_timer_state[MPID_STATE_VIA_POST_WRITE].name = "via_post_write";

    g_timer_state[MPID_STATE_VIA_RDMA_CAN_CONNECT].name = "via_rdma_can_connect";
    g_timer_state[MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD].name = "via_rdma_get_business_card";
    g_timer_state[MPID_STATE_VIA_RDMA_INIT].name = "via_rdma_init";
    g_timer_state[MPID_STATE_VIA_RDMA_FINALIZE].name = "via_rdma_finalize";
    g_timer_state[MPID_STATE_VIA_RDMA_MAKE_PROGRESS].name = "via_rdma_make_progress";
    g_timer_state[MPID_STATE_VIA_RDMA_POST_CONNECT].name = "via_rdma_post_connect";
    g_timer_state[MPID_STATE_VIA_RDMA_POST_READ].name = "via_rdma_post_read";
    g_timer_state[MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED].name = "via_rdma_merge_with_unexpected";
    g_timer_state[MPID_STATE_VIA_RDMA_POST_WRITE].name = "via_rdma_post_write";

    return 0;
}

#endif /* USE_LOGGING == MPID_LOGGING_DLOG */


/* This section of code is for the RLOG logging library */
#if (USE_LOGGING == MPID_LOGGING_RLOG)

#include <math.h>

/* utility funcions */
#ifndef RGB
#define RGB(r,g,b)      ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#endif

static unsigned long getColorRGB(double fraction, double intensity, unsigned char *r, unsigned char *g, unsigned char *b)
{
    double red, green, blue;
    double dtemp;

    fraction = fabs(modf(fraction, &dtemp));
    
    if (intensity > 2.0)
	intensity = 2.0;
    if (intensity < 0.0)
	intensity = 0.0;
    
    dtemp = 1.0/6.0;
    
    if (fraction < 1.0/6.0)
    {
	red = 1.0;
	green = fraction / dtemp;
	blue = 0.0;
    }
    else
    {
	if (fraction < 1.0/3.0)
	{
	    red = 1.0 - ((fraction - dtemp) / dtemp);
	    green = 1.0;
	    blue = 0.0;
	}
	else
	{
	    if (fraction < 0.5)
	    {
		red = 0.0;
		green = 1.0;
		blue = (fraction - (dtemp*2.0)) / dtemp;
	    }
	    else
	    {
		if (fraction < 2.0/3.0)
		{
		    red = 0.0;
		    green = 1.0 - ((fraction - (dtemp*3.0)) / dtemp);
		    blue = 1.0;
		}
		else
		{
		    if (fraction < 5.0/6.0)
		    {
			red = (fraction - (dtemp*4.0)) / dtemp;
			green = 0.0;
			blue = 1.0;
		    }
		    else
		    {
			red = 1.0;
			green = 0.0;
			blue = 1.0 - ((fraction - (dtemp*5.0)) / dtemp);
		    }
		}
	    }
	}
    }
    
    if (intensity > 1)
    {
	intensity = intensity - 1.0;
	red = red + ((1.0 - red) * intensity);
	green = green + ((1.0 - green) * intensity);
	blue = blue + ((1.0 - blue) * intensity);
    }
    else
    {
	red = red * intensity;
	green = green * intensity;
	blue = blue * intensity;
    }
    
    *r = (unsigned char)(red * 255.0);
    *g = (unsigned char)(green * 255.0);
    *b = (unsigned char)(blue * 255.0);

    return RGB(*r,*g,*b);
}

static unsigned long random_color(unsigned char *r, unsigned char *g, unsigned char *b)
{
    double d1, d2;

    d1 = (double)rand() / (double)RAND_MAX;
    d2 = (double)rand() / (double)RAND_MAX;

    return getColorRGB(d1, d2 + 0.5, r, g, b);
}

static char random_color_str[40];
static char *get_random_color_str()
{
    unsigned char r,g,b;
    random_color(&r, &g, &b);
    sprintf(random_color_str, "%3d %3d %3d", (int)r, (int)g, (int)b);
    return random_color_str;
}

int MPIDU_Describe_timer_states()
{
    /* mpid functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_ISEND, "MPID_Isend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_IRECV, "MPID_Irecv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_SEND, "MPID_Send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_RECV, "MPID_Recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_PROGRESS_TEST, "MPID_Progress_test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_ABORT, "MPID_Abort", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_CLOSE_PORT, "MPID_Close_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_ACCEPT, "MPID_Comm_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_CONNECT, "MPID_Comm_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_DISCONNECT, "MPID_Comm_disconnect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_SPAWN_MULTIPLE, "MPID_Comm_spawn_multiple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_OPEN_PORT, "MPID_Open_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_PROGRESS_WAIT, "MPID_Progress_wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_REQUEST_RELEASE, "MPID_Request_release", get_random_color_str());

    /* bsocket functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BREAD, "bread", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BREADV, "breadv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BWRITE, "bwrite", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BWRITEV, "bwritev", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BSELECT, "bselect", get_random_color_str());

    /* mm functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_OPEN_PORT, "mm_open_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CLOSE_PORT, "mm_close_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_ACCEPT, "mm_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CONNECT, "mm_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_SEND, "mm_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_RECV, "mm_recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CLOSE, "mm_close", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_REQUEST_ALLOC, "mm_request_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_REQUEST_FREE, "mm_request_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_INIT, "mm_car_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_FINALIZE, "mm_car_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_ALLOC, "mm_car_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_FREE, "mm_car_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_INIT, "mm_vc_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FINALIZE, "mm_vc_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FROM_COMMUNICATOR, "mm_vc_from_communicator", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FROM_CONTEXT, "mm_vc_from_context", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_ALLOC, "mm_vc_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_CONNECT_ALLOC, "mm_vc_connect_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FREE, "mm_vc_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CHOOSE_BUFFER, "mm_choose_buffer", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_RESET_CARS, "mm_reset_cars", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_GET_BUFFERS_TMP, "mm_get_buffers_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_RELEASE_BUFFERS_TMP, "mm_release_buffers_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_GET_BUFFERS_VEC, "mm_get_buffers_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VEC_BUFFER_INIT, "vec_buffer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TMP_BUFFER_INIT, "tmp_buffer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SIMPLE_BUFFER_INIT, "simple_buffer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_RECV, "mm_post_recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_SEND, "mm_post_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_RNDV_DATA_SEND, "mm_post_rndv_data_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND, "mm_post_rndv_clear_to_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_TEST, "mm_cq_test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_WAIT, "mm_cq_wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_ENQUEUE, "mm_cq_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CREATE_POST_UNEX, "mm_create_post_unex", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND, "mm_enqueue_request_to_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_READ_HEAD_CAR, "cq_handle_read_head_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_READ_DATA_CAR, "cq_handle_read_data_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_READ_CAR, "cq_handle_read_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_WRITE_HEAD_CAR, "cq_handle_write_head_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_WRITE_DATA_CAR, "cq_handle_write_data_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_WRITE_CAR, "cq_handle_write_car", get_random_color_str());

    /* xfer functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_INIT, "xfer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_OP, "xfer_recv_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_MOP_OP, "xfer_recv_mop_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_FORWARD_OP, "xfer_recv_forward_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_MOP_FORWARD_OP, "xfer_mop_forward_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_FORWARD_OP, "xfer_forward_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_SEND_OP, "xfer_send_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_REPLICATE_OP, "xfer_replicate_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_START, "xfer_start", get_random_color_str());

    /* method functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_INIT, "tcp_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_FINALIZE, "tcp_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_ACCEPT_CONNECTION, "tcp_accept_connection", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_GET_BUSINESS_CARD, "tcp_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAN_CONNECT, "tcp_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_CONNECT, "tcp_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_READ, "tcp_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_WITH_UNEXPECTED, "tcp_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_WRITE, "tcp_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MAKE_PROGRESS, "tcp_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_ENQUEUE, "tcp_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_DEQUEUE, "tcp_car_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_DEQUEUE_WRITE, "tcp_dequeue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_RESET_CAR, "tcp_reset_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_READ_PKT, "tcp_post_read_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ, "tcp_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE, "tcp_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_SHM, "tcp_read_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_VIA, "tcp_read_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_VIA_RDMA, "tcp_read_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_VEC, "tcp_read_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_TMP, "tcp_read_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_CONNECTING, "tcp_read_connecting", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_SHM, "tcp_write_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_VIA, "tcp_write_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_VIA_RDMA, "tcp_write_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_VEC, "tcp_write_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_TMP, "tcp_write_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_SHM, "tcp_stuff_vector_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_VIA, "tcp_stuff_vector_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_VIA_RDMA, "tcp_stuff_vector_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_VEC, "tcp_stuff_vector_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_TMP, "tcp_stuff_vector_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_AGGRESSIVE, "tcp_write_aggressive", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_HEAD_ENQUEUE, "tcp_car_head_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_SETUP_PACKET_CAR, "tcp_setup_packet_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_UPDATE_CAR_NUM_WRITTEN, "tcp_update_car_num_written", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_UNEXPECTED_DATA, "tcp_merge_unexpected_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_SHM, "tcp_merge_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_VIA, "tcp_merge_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_VIA_RDMA, "tcp_merge_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_VEC, "tcp_merge_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_TMP, "tcp_merge_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_SIMPLE, "tcp_merge_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_WITH_POSTED, "tcp_merge_with_posted", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_HEADER, "tcp_read_header", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_DATA, "tcp_read_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_SIMPLE, "tcp_read_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_SIMPLE, "tcp_write_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_SIMPLE, "tcp_stuff_vector_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_FIND_IN_QUEUE, "find_in_queue", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_CAN_CONNECT, "shm_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_GET_BUSINESS_CARD, "shm_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_INIT, "shm_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_FINALIZE, "shm_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_MAKE_PROGRESS, "shm_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_ALLOC, "shm_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_FREE, "shm_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_GET_MEM_SYNC, "shm_get_mem_sync", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_RELEASE_MEM, "shm_release_mem", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_POST_CONNECT, "shm_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_POST_READ, "shm_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_MERGE_WITH_UNEXPECTED, "shm_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_POST_WRITE, "shm_post_write", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_CAR_ENQUEUE, "packer_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_CAR_DEQUEUE, "packer_car_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_INIT, "packer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_FINALIZE, "packer_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_MAKE_PROGRESS, "packer_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_POST_READ, "packer_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED, "packer_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_POST_WRITE, "packer_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_RESET_CAR, "packer_reset_car", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_CAR_ENQUEUE, "unpacker_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_CAR_DEQUEUE, "unpacker_car_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_INIT, "unpacker_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_FINALIZE, "unpacker_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_MAKE_PROGRESS, "unpacker_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_SHM, "unpacker_write_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_VIA, "unpacker_write_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_VIA_RDMA, "unpacker_write_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_VEC, "unpacker_write_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_TMP, "unpacker_write_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_POST_READ, "unpacker_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_MERGE_WITH_UNEXPECTED, "unpacker_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_POST_WRITE, "unpacker_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_RESET_CAR, "unpacker_reset_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_SIMPLE, "unpacker_write_simple", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_CAN_CONNECT, "via_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_GET_BUSINESS_CARD, "via_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_INIT, "via_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_FINALIZE, "via_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_MAKE_PROGRESS, "via_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_POST_CONNECT, "via_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_POST_READ, "via_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_MERGE_WITH_UNEXPECTED, "via_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_POST_WRITE, "via_post_write", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_CAN_CONNECT, "via_rdma_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD, "via_rdma_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_INIT, "via_rdma_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_FINALIZE, "via_rdma_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_MAKE_PROGRESS, "via_rdma_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_POST_CONNECT, "via_rdma_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_POST_READ, "via_rdma_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED, "via_rdma_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_POST_WRITE, "via_rdma_post_write", get_random_color_str());

    return 0;
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */
