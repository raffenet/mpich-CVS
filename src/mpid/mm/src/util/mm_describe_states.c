/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifdef HAVE_TIMING

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
    g_timer_state[MPID_STATE_SELECT].name = "select";

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
    g_timer_state[MPID_STATE_MM_CQ_HANDLE_READ_HEAD_CAR].name = "mm_cq_handle_read_head_car";
    g_timer_state[MPID_STATE_MM_CQ_HANDLE_READ_DATA_CAR].name = "mm_cq_handle_read_data_car";
    g_timer_state[MPID_STATE_MM_CQ_HANDLE_READ_CAR].name = "mm_cq_handle_read_car";
    g_timer_state[MPID_STATE_MM_CQ_HANDLE_WRITE_HEAD_CAR].name = "mm_cq_handle_write_head_car";
    g_timer_state[MPID_STATE_MM_CQ_HANDLE_WRITE_DATA_CAR].name = "mm_cq_handle_write_data_car";
    g_timer_state[MPID_STATE_MM_CQ_HANDLE_WRITE_CAR].name = "mm_cq_handle_write_car";
    g_timer_state[MPID_STATE_MM_MAKE_PROGRESS].name = "mm_make_progress";

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
    g_timer_state[MPID_STATE_TCP_MERGE_IB].name = "tcp_merge_ib";
    g_timer_state[MPID_STATE_TCP_READ_IB].name = "tcp_read_ib";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_IB].name = "tcp_stuff_vector_ib";

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
    g_timer_state[MPID_STATE_UNPACKER_WRITE_IB].name = "unpacker_write_ib";

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

    g_timer_state[MPID_STATE_SOCKET_CAN_CONNECT].name = "socket_can_connect";
    g_timer_state[MPID_STATE_SOCKET_CAR_HEAD_ENQUEUE].name = "socket_car_head_enqueue";
    g_timer_state[MPID_STATE_SOCKET_CAR_HEAD_ENQUEUE_READ].name = "socket_car_head_enqueue_read";
    g_timer_state[MPID_STATE_SOCKET_CAR_HEAD_ENQUEUE_WRITE].name = "socket_car_head_enqueue_write";
    g_timer_state[MPID_STATE_SOCKET_CAR_ENQUEUE].name = "socket_car_enqueue";
    g_timer_state[MPID_STATE_SOCKET_CAR_ENQUEUE_READ].name = "socket_car_enqueue_read";
    g_timer_state[MPID_STATE_SOCKET_CAR_ENQUEUE_WRITE].name = "socket_car_enqueue_write";
    g_timer_state[MPID_STATE_SOCKET_CAR_DEQUEUE_WRITE].name = "socket_car_dequeue_write";
    g_timer_state[MPID_STATE_SOCKET_CAR_DEQUEUE_READ].name = "socket_car_dequeue_read";
    g_timer_state[MPID_STATE_SOCKET_GET_BUSINESS_CARD].name = "socket_get_business_card";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_SHM].name = "socket_handle_written_shm";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_VIA].name = "socket_handle_written_via";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_VIA_RDMA].name = "socket_handle_written_via_rdma";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_VEC].name = "socket_handle_written_vec";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_TMP].name = "socket_handle_written_tmp";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_SIMPLE].name = "socket_handle_written_simple";
    g_timer_state[MPID_STATE_SOCKET_INIT].name = "socket_init";
    g_timer_state[MPID_STATE_SOCKET_FINALIZE].name = "socket_finalize";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_ACCEPT].name = "socket_handle_accept";
    g_timer_state[MPID_STATE_SOCKET_MAKE_PROGRESS].name = "socket_make_progress";
    g_timer_state[MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA].name = "socket_merge_unexpected_data";
    g_timer_state[MPID_STATE_SOCKET_MERGE_SHM].name = "socket_merge_shm";
    g_timer_state[MPID_STATE_SOCKET_MERGE_VIA].name = "socket_merge_via";
    g_timer_state[MPID_STATE_SOCKET_MERGE_VIA_RDMA].name = "socket_merge_via_rdma";
    g_timer_state[MPID_STATE_SOCKET_MERGE_IB].name = "socket_merge_ib";
    g_timer_state[MPID_STATE_SOCKET_MERGE_VEC].name = "socket_merge_vec";
    g_timer_state[MPID_STATE_SOCKET_MERGE_TMP].name = "socket_merge_tmp";
    g_timer_state[MPID_STATE_SOCKET_MERGE_SIMPLE].name = "socket_merge_simple";
    g_timer_state[MPID_STATE_SOCKET_MERGE_WITH_POSTED].name = "socket_merge_with_posted";
    g_timer_state[MPID_STATE_SOCKET_MERGE_WITH_UNEXPECTED].name = "socket_merge_with_unexpected";
    g_timer_state[MPID_STATE_SOCKET_POST_CONNECT].name = "socket_post_connect";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_CONNECT].name = "socket_handle_connect";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK].name = "socket_handle_written_ack";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN_CONTEXT_PKT].name = "socket_handle_written_context_pkt";
    g_timer_state[MPID_STATE_SOCKET_POST_READ].name = "socket_post_read";
    g_timer_state[MPID_STATE_SOCKET_POST_READ_PKT].name = "socket_post_read_pkt";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_ACK].name = "socket_handle_read_ack";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ].name = "socket_handle_read";
    g_timer_state[MPID_STATE_SOCKET_POST_WRITE].name = "socket_post_write";
    g_timer_state[MPID_STATE_SOCKET_READ_HEADER].name = "socket_read_header";
    g_timer_state[MPID_STATE_SOCKET_READ_DATA].name = "socket_read_data";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_DATA].name = "socket_handle_read_data";
    g_timer_state[MPID_STATE_SOCKET_READ_SHM].name = "socket_read_shm";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_SHM].name = "socket_handle_shm";
    g_timer_state[MPID_STATE_SOCKET_READ_VIA].name = "socket_read_via";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_VIA].name = "socket_handle_read_via";
    g_timer_state[MPID_STATE_SOCKET_READ_VIA_RDMA].name = "socket_read_via_rdma";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_VIA_RDMA].name = "socket_handle_read_via_rdma";
    g_timer_state[MPID_STATE_SOCKET_READ_IB].name = "socket_read_ib";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_IB].name = "socket_handle_read_ib";
    g_timer_state[MPID_STATE_SOCKET_READ_VEC].name = "socket_read_vec";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_VEC].name = "socket_handle_read_vec";
    g_timer_state[MPID_STATE_SOCKET_READ_TMP].name = "socket_read_tmp";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_TMP].name = "socket_handle_read_tmp";
    g_timer_state[MPID_STATE_SOCKET_READ_SIMPLE].name = "socket_read_simple";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_SIMPLE].name = "socket_handle_read_simple";
    g_timer_state[MPID_STATE_SOCKET_RESET_CAR].name = "socket_reset_car";
    g_timer_state[MPID_STATE_SOCKET_SETUP_PACKET_CAR].name = "socket_setup_packet_car";
    g_timer_state[MPID_STATE_SOCKET_WRITE].name = "socket_write";
    g_timer_state[MPID_STATE_SOCKET_WRITE_SHM].name = "socket_write_shm";
    g_timer_state[MPID_STATE_SOCKET_WRITE_VIA].name = "socket_write_via";
    g_timer_state[MPID_STATE_SOCKET_WRITE_VIA_RDMA].name = "socket_write_via_rdma";
    g_timer_state[MPID_STATE_SOCKET_WRITE_VEC].name = "socket_write_vec";
    g_timer_state[MPID_STATE_SOCKET_WRITE_TMP].name = "socket_write_tmp";
    g_timer_state[MPID_STATE_SOCKET_WRITE_SIMPLE].name = "socket_write_simple";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_SHM].name = "socket_stuff_vector_shm";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_VIA].name = "socket_stuff_vector_via";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_VIA_RDMA].name = "socket_stuff_vector_via_rdma";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_VEC].name = "socket_stuff_vector_vec";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_TMP].name = "socket_stuff_vector_tmp";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_SIMPLE].name = "socket_stuff_vector_simple";
    g_timer_state[MPID_STATE_SOCKET_STUFF_VECTOR_IB].name = "socket_stuff_vector_ib";
    g_timer_state[MPID_STATE_SOCKET_UPDATE_CAR_NUM_WRITTEN].name = "socket_update_car_num_written";
    g_timer_state[MPID_STATE_SOCKET_WRITE_AGGRESSIVE].name = "socket_write_aggressive";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_WRITTEN].name = "socket_handle_written";
    g_timer_state[MPID_STATE_SOCKET_HANDLE_READ_CONTEXT_PKT].name = "socket_handle_read_context_pkt";

    g_timer_state[MPID_STATE_IB_CAN_CONNECT].name = "ib_can_connect";
    g_timer_state[MPID_STATE_IB_GET_BUSINESS_CARD].name = "ib_get_business_card";
    g_timer_state[MPID_STATE_IB_INIT].name = "ib_init";
    g_timer_state[MPID_STATE_IB_FINALIZE].name = "ib_finalize";
    g_timer_state[MPID_STATE_IB_HANDLE_ACCEPT].name = "ib_handle_accept";
    g_timer_state[MPID_STATE_IB_MAKE_PROGRESS].name = "ib_make_progress";
    g_timer_state[MPID_STATE_IB_MERGE_UNEXPECTED_DATA].name = "ib_merge_unexpected_data";
    g_timer_state[MPID_STATE_IB_MERGE_SHM].name = "ib_merge_shm";
    g_timer_state[MPID_STATE_IB_MERGE_VIA].name = "ib_merge_via";
    g_timer_state[MPID_STATE_IB_MERGE_VIA_RDMA].name = "ib_merge_via_rdma";
    g_timer_state[MPID_STATE_IB_MERGE_IB].name = "ib_merge_ib";
    g_timer_state[MPID_STATE_IB_MERGE_VEC].name = "ib_merge_vec";
    g_timer_state[MPID_STATE_IB_MERGE_TMP].name = "ib_merge_tmp";
    g_timer_state[MPID_STATE_IB_MERGE_SIMPLE].name = "ib_merge_simple";
    g_timer_state[MPID_STATE_IB_MERGE_WITH_POSTED].name = "ib_merge_with_posted";
    g_timer_state[MPID_STATE_IB_MERGE_WITH_UNEXPECTED].name = "ib_merge_with_unexpected";
    g_timer_state[MPID_STATE_IB_POST_CONNECT].name = "ib_post_connect";
    g_timer_state[MPID_STATE_IB_HANDLE_CONNECT].name = "ib_handle_connect";
    g_timer_state[MPID_STATE_IB_HANDLE_WRITTEN_ACK].name = "ib_handle_written_ack";
    g_timer_state[MPID_STATE_IB_HANDLE_WRITTEN_CONTEXT_PKT].name = "ib_handle_written_context_pkt";
    g_timer_state[MPID_STATE_IB_POST_READ].name = "ib_post_read";
    g_timer_state[MPID_STATE_IB_POST_READ_PKT].name = "ib_post_read_pkt";
    g_timer_state[MPID_STATE_IB_HANDLE_READ_ACK].name = "ib_handle_read_ack";
    g_timer_state[MPID_STATE_IB_HANDLE_READ_CONTEXT_PKT].name = "ib_handle_read_context_pkt";
    g_timer_state[MPID_STATE_IB_HANDLE_READ].name = "ib_handle_read";
    g_timer_state[MPID_STATE_IB_POST_WRITE].name = "ib_post_write";
    g_timer_state[MPID_STATE_IB_CAR_HEAD_ENQUEUE].name = "ib_car_head_enqueue";
    g_timer_state[MPID_STATE_IB_CAR_ENQUEUE].name = "ib_car_enqueue";
    g_timer_state[MPID_STATE_IB_CAR_DEQUEUE_WRITE].name = "ib_dequeue_write";
    g_timer_state[MPID_STATE_IB_CAR_DEQUEUE].name = "ib_dequeue";
    g_timer_state[MPID_STATE_IB_RESET_CAR].name = "ib_reset_car";
    g_timer_state[MPID_STATE_IB_SETUP_PACKET_CAR].name = "ib_setup_packet_car";

    g_timer_state[MPID_STATE_SOCK_INIT].name = "sock_init";
    g_timer_state[MPID_STATE_SOCK_FINALIZE].name = "sock_finalize";
    g_timer_state[MPID_STATE_SOCK_CREATE_SET].name = "sock_create_set";
    g_timer_state[MPID_STATE_SOCK_DESTROY_SET].name = "sock_destroy_set";
    g_timer_state[MPID_STATE_SOCK_LISTEN].name = "sock_listen";
    g_timer_state[MPID_STATE_SOCK_POST_CONNECT].name = "sock_post_connect";
    g_timer_state[MPID_STATE_SOCK_ACCEPT].name = "sock_accept";
    g_timer_state[MPID_STATE_SOCK_POST_CLOSE].name = "sock_post_close";
    g_timer_state[MPID_STATE_SOCK_WAIT].name = "sock_wait";
    g_timer_state[MPID_STATE_SOCK_SET_USER_PTR].name = "sock_set_user_ptr";
    g_timer_state[MPID_STATE_SOCK_READ].name = "sock_read";
    g_timer_state[MPID_STATE_SOCK_READV].name = "sock_readv";
    g_timer_state[MPID_STATE_SOCK_WRITE].name = "sock_write";
    g_timer_state[MPID_STATE_SOCK_WRITEV].name = "sock_writev";
    g_timer_state[MPID_STATE_SOCK_POST_READ].name = "sock_post_read";
    g_timer_state[MPID_STATE_SOCK_POST_READV].name = "sock_post_readv";
    g_timer_state[MPID_STATE_SOCK_POST_WRITE].name = "sock_write";
    g_timer_state[MPID_STATE_SOCK_POST_WRITEV].name = "sock_writev";
    g_timer_state[MPID_STATE_SOCK_EASY_RECEIVE].name = "sock_easy_receive";
    g_timer_state[MPID_STATE_SOCK_EASY_SEND].name = "sock_easy_send";

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
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_BSEND_INIT, "MPID_Bsend_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_STARTALL, "MPID_Startall", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_WIN_FENCE, "MPID_Win_fence", get_random_color_str());

    /* bsocket functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BACCEPT, "baccept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BBIND, "bbind", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BCLOSE, "bclose", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BCLR, "bclr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BCONNECT, "bconnect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BCOPYSET, "bcopyset", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_ACCEPT, "beasy_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_CLOSESOCKET, "beasy_closesocket", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_CONNECT, "beasy_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_CREATE, "beasy_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_GET_IP, "beasy_get_ip", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_GET_IP_STRING, "beasy_get_ip_string", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_GET_SOCK_INFO, "beasy_get_sock_info", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_RECEIVE, "beasy_receive", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_RECEIVE_SOME, "beasy_receive_some", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_RECEIVE_TIMEOUT, "beasy_receive_timeout", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BEASY_SEND, "beasy_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BGETSOCKNAME, "bgetsockname", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BLISTEN, "blisten", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BMAKE_BLOCKING, "bmake_blocking", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BMAKE_NONBLOCKING, "bmake_nonblocking", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BREAD, "bread", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BREADV, "breadv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BSELECT, "bselect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BSET, "bset", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BSETSOCKOPT, "bsetsockopt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BSOCKET, "bsocket", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BWRITE, "bwrite", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BWRITEV, "bwritev", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_READ, "read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_READV, "readv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SELECT, "select", get_random_color_str());

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
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_HANDLE_READ_HEAD_CAR, "mm_cq_handle_read_head_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_HANDLE_READ_DATA_CAR, "mm_cq_handle_read_data_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_HANDLE_READ_CAR, "mm_cq_handle_read_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_HANDLE_WRITE_HEAD_CAR, "mm_cq_handle_write_head_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_HANDLE_WRITE_DATA_CAR, "mm_cq_handle_write_data_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_HANDLE_WRITE_CAR, "mm_cq_handle_write_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_MAKE_PROGRESS, "mm_make_progress", get_random_color_str());

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
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_IB, "tcp_merge_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_IB, "tcp_read_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_IB, "tcp_stuff_vector_ib", get_random_color_str());

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
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_IB, "unpacker_write_ib", get_random_color_str());

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

    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAN_CONNECT, "socket_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_HEAD_ENQUEUE, "socket_car_head_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_HEAD_ENQUEUE_READ, "socket_car_head_enqueue_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_HEAD_ENQUEUE_WRITE, "socket_car_head_enqueue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_ENQUEUE, "socket_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_ENQUEUE_READ, "socket_car_enqueue_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_ENQUEUE_WRITE, "socket_car_enqueue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_DEQUEUE_WRITE, "socket_car_dequeue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_CAR_DEQUEUE_READ, "socket_car_dequeue_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_GET_BUSINESS_CARD, "socket_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_SHM, "socket_handle_written_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_VIA, "socket_handle_written_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_VIA_RDMA, "socket_handle_written_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_VEC, "socket_handle_written_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_TMP, "socket_handle_written_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_SIMPLE, "socket_handle_written_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_INIT, "socket_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_FINALIZE, "socket_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_ACCEPT, "socket_handle_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MAKE_PROGRESS, "socket_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_UNEXPECTED_DATA, "socket_merge_unexpected_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_SHM, "socket_merge_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_VIA, "socket_merge_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_VIA_RDMA, "socket_merge_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_IB, "socket_merge_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_VEC, "socket_merge_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_TMP, "socket_merge_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_SIMPLE, "socket_merge_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_WITH_POSTED, "socket_merge_with_posted", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_MERGE_WITH_UNEXPECTED, "socket_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_POST_CONNECT, "socket_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_CONNECT, "socket_handle_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK, "socket_handle_written_ack", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN_CONTEXT_PKT, "socket_handle_written_context_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_POST_READ, "socket_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_POST_READ_PKT, "socket_post_read_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_ACK, "socket_handle_read_ack", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ, "socket_handle_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_POST_WRITE, "socket_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_HEADER, "socket_read_header", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_DATA, "socket_read_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_DATA, "socket_handle_read_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_SHM, "socket_read_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_SHM, "socket_handle_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_VIA, "socket_read_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_VIA, "socket_handle_read_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_VIA_RDMA, "socket_read_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_VIA_RDMA, "socket_handle_read_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_IB, "socket_read_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_IB, "socket_handle_read_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_VEC, "socket_read_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_VEC, "socket_handle_read_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_TMP, "socket_read_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_TMP, "socket_handle_read_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_READ_SIMPLE, "socket_read_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_SIMPLE, "socket_handle_read_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_RESET_CAR, "socket_reset_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_SETUP_PACKET_CAR, "socket_setup_packet_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE, "socket_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_SHM, "socket_write_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_VIA, "socket_write_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_VIA_RDMA, "socket_write_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_VEC, "socket_write_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_TMP, "socket_write_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_SIMPLE, "socket_write_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_SHM, "socket_stuff_vector_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_VIA, "socket_stuff_vector_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_VIA_RDMA, "socket_stuff_vector_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_VEC, "socket_stuff_vector_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_TMP, "socket_stuff_vector_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_SIMPLE, "socket_stuff_vector_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_STUFF_VECTOR_IB, "socket_stuff_vector_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_UPDATE_CAR_NUM_WRITTEN, "socket_update_car_num_written", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_WRITE_AGGRESSIVE, "socket_write_aggressive", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_WRITTEN, "socket_handle_written", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCKET_HANDLE_READ_CONTEXT_PKT, "socket_handle_read_context_pkt", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAN_CONNECT, "ib_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_GET_BUSINESS_CARD, "ib_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_INIT, "ib_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_FINALIZE, "ib_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_ACCEPT, "ib_handle_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MAKE_PROGRESS, "ib_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_UNEXPECTED_DATA, "ib_merge_unexpected_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_SHM, "ib_merge_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_VIA, "ib_merge_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_VIA_RDMA, "ib_merge_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_IB, "ib_merge_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_VEC, "ib_merge_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_TMP, "ib_merge_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_SIMPLE, "ib_merge_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_WITH_POSTED, "ib_merge_with_posted", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_MERGE_WITH_UNEXPECTED, "ib_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_POST_CONNECT, "ib_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_CONNECT, "ib_handle_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_WRITTEN_ACK, "ib_handle_written_ack", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_WRITTEN_CONTEXT_PKT, "ib_handle_written_context_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_POST_READ, "ib_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_POST_READ_PKT, "ib_post_read_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_ACK, "ib_handle_read_ack", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_CONTEXT_PKT, "ib_handle_read_context_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ, "ib_handle_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_POST_WRITE, "ib_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAR_HEAD_ENQUEUE, "ib_car_head_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAR_ENQUEUE, "ib_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAR_DEQUEUE_WRITE, "ib_dequeue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAR_DEQUEUE, "ib_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_RESET_CAR, "ib_reset_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_SETUP_PACKET_CAR, "ib_setup_packet_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAR_ENQUEUE_READ, "ib_car_enqueue_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_CAR_ENQUEUE_WRITE, "ib_car_enqueue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_READ_DATA, "ib_read_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_DATA, "ib_handle_read_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_READ_IB, "ib_read_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_IB, "ib_handle_read_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_READ_VEC, "ib_read_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_VEC, "ib_handle_read_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_READ_TMP, "ib_read_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_TMP, "ib_handle_read_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_READ_SIMPLE, "ib_read_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_READ_SIMPLE, "ib_handle_read_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_STUFF_VECTOR_VEC, "ib_stuff_vector_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_STUFF_VECTOR_TMP, "ib_stuff_vector_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_STUFF_VECTOR_SIMPLE, "ib_stuff_vector_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_STUFF_VECTOR_IB, "ib_stuff_vector_ib", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_UPDATE_CAR_NUM_WRITTEN, "ib_update_car_num_written", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_WRITE_AGGRESSIVE, "ib_write_aggressive", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IB_HANDLE_WRITTEN, "ib_handle_written", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_INIT, "ibu_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_FINALIZE, "ibu_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_CREATE_SET, "ibu_create_set", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_DESTROY_SET, "ibu_destroy_set", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_WAIT, "ibu_wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_SET_USER_PTR, "ibu_set_user_ptr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_POST_READ, "ibu_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_POST_READV, "ibu_post_readv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_POST_WRITE, "ibu_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_IBU_POST_WRITEV, "ibu_post_writev", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_INIT, "sock_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_FINALIZE, "sock_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_CREATE_SET, "sock_create_set", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_DESTROY_SET, "sock_destroy_set", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_LISTEN, "sock_listen", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_POST_CONNECT, "sock_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_ACCEPT, "sock_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_POST_CLOSE, "sock_post_close", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_WAIT, "sock_wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_SET_USER_PTR, "sock_set_user_ptr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_READ, "sock_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_READV, "sock_readv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_WRITE, "sock_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_WRITEV, "sock_writev", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_POST_READ, "sock_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_POST_READV, "sock_post_readv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_POST_WRITE, "sock_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_POST_WRITEV, "sock_writev", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_EASY_RECEIVE, "sock_easy_receive", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SOCK_EASY_SEND, "sock_easy_send", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_COPYSET, "copyset", get_random_color_str());

    return 0;
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
