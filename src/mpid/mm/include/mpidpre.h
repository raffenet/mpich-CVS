/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIDPRE_H
#define MPIDPRE_H

#include "mpi.h"
#include "mpidconf.h"
#include "mpid_datatype.h"
#include "mpid_dataloop.h"
#include "mpiimpl.h"

#ifdef WITH_METHOD_TCP
#include "mm_tcp_pre.h"
#endif
#ifdef WITH_METHOD_SOCKET
#include "mm_socket_pre.h"
#endif
#ifdef WITH_METHOD_SHM
#include "mm_shm_pre.h"
#endif
#ifdef WITH_METHOD_VIA
#include "mm_via_pre.h"
#endif
#ifdef WITH_METHOD_VIA_RDMA
#include "mm_via_rdma_pre.h"
#endif
#ifdef WITH_NEW_METHOD
#include "mm_new_pre.h"
#endif

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#define MPID_IOV         WSABUF
#define MPID_IOV_LEN     len
#define MPID_IOV_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_IOV         struct iovec
#define MPID_IOV_LEN     iov_len
#define MPID_IOV_BUF     iov_base
#endif
#define MPID_IOV_LIMIT   16

/* Buffer type */
typedef enum MM_BUFFER_TYPE {
    MM_NULL_BUFFER,
    MM_SIMPLE_BUFFER,
    MM_TMP_BUFFER,
    MM_VEC_BUFFER,
#ifdef WITH_METHOD_SHM
    MM_SHM_BUFFER,
#endif
#ifdef WITH_METHOD_VIA
    MM_VIA_BUFFER,
#endif
#ifdef WITH_METHOD_VIA_RDMA
    MM_VIA_RDMA_BUFFER,
#endif
#ifdef WITH_METHOD_NEW
    MM_NEW_METHOD_BUFFER,
#endif
    MM_END_MARKER_BUFFER
} MM_BUFFER_TYPE;

/* Packet header type */
typedef enum { 
    MPID_INVALID_PKT,
    MPID_EAGER_PKT,
    MPID_RNDV_REQUEST_TO_SEND_PKT,
    MPID_RNDV_CLEAR_TO_SEND_PKT,
    MPID_RNDV_DATA_PKT,
    MPID_RDMA_ACK_PKT,
    MPID_RDMA_DATA_ACK_PKT,
    MPID_RDMA_REQUEST_DATA_ACK_PKT,
    MPID_CONNECT_PKT,
    MPID_ACK_PKT
} MPID_Packet_type;

/* Communication agent request type */
typedef int MM_CAR_TYPE;
#define MM_NULL_CAR      ( 0x0 )
#define MM_HEAD_CAR      ( 0x1 )
#define MM_READ_CAR      ( 0x1 << 1 )
#define MM_WRITE_CAR     ( 0x1 << 2 )
#define MM_PACKER_CAR    ( 0x1 << 3 )
#define MM_UNPACKER_CAR  ( 0x1 << 4 )

/* packet definitions */
typedef struct MPID_Header_pkt
{
    MPID_Packet_type type;
    int context;
    int tag;
    int src;
    int size;
    struct MM_Car *sender_car_ptr;
} MPID_Header_pkt;

typedef struct MPID_Connect_pkt
{
    MPID_Packet_type type;
    int rank;
    int context;
    char ack_in, ack_out;
} MPID_Connect_pkt;

typedef struct MPID_Rndv_clear_to_send_pkt
{
    MPID_Packet_type type;
    struct MM_Car *sender_car_ptr;
    struct MM_Car *receiver_car_ptr;
} MPID_Rndv_clear_to_send_pkt;

typedef struct MPID_Rndv_data_pkt
{
    MPID_Packet_type type;
    int size;
    struct MM_Car *receiver_car_ptr;
} MPID_Rndv_data_pkt;

#ifdef WITH_VIA_RDMA_METHOD
typedef struct MPID_Rdma_ack_pkt
{
    MPID_Packet_type type;
} MPID_Rdma_ack_pkt;

typedef struct MPID_Rdma_data_ack_pkt
{
    MPID_Packet_type type;
} MPID_Rdma_data_ack_pkt;

typedef struct MPID_Rdma_request_data_ack_pkt
{
    MPID_Packet_type type;
} MPID_Rdma_request_data_ack_pkt;
#endif

typedef struct MPID_Packet
{
    union MPID_Packet_contents
    {
	MPID_Packet_type type;
	MPID_Header_pkt hdr;
	MPID_Connect_pkt connect;
	MPID_Rndv_clear_to_send_pkt cts;
	MPID_Rndv_data_pkt rdata;
#ifdef WITH_VIA_RDMA_METHOD
	MPID_Rdma_ack_pkt rdma_ack;
	MPID_Rdma_data_ack_pkt rdma_data_ack;
	MPID_Rdma_request_data_ack_pkt rdma_req_data_ack;
#endif
    } u;
} MPID_Packet;

typedef union MM_Segment_buffer
{
    MM_BUFFER_TYPE type;
    /*
    struct mm_segment_tmp
    {
	MM_BUFFER_TYPE type;
	void *buf[2];
	int len[2];
	int cur_buf;
	int num_read;
	int min_num_written;
    } tmp;
    */
    struct mm_segment_simple
    {
	MM_BUFFER_TYPE type;
	void *buf;
	int len;
	int num_read;
    } simple;
    struct mm_segment_tmp
    {
	MM_BUFFER_TYPE type;
	void *buf;
	int len;
	int num_read;
    } tmp;
    struct mm_segment_vec
    {
	MM_BUFFER_TYPE type;
	MPID_IOV vec[MPID_IOV_LIMIT];
	int vec_size;
	int num_read;
	int first, last, segment_last;
	int buf_size;
	int num_cars, num_cars_outstanding;
    } vec;
#ifdef WITH_METHOD_SHM
    struct mm_segment_shm
    {
	MM_BUFFER_TYPE type;
	void *shm_ptr;
	int num_read;
    } shm;
#endif
#ifdef WITH_METHOD_VIA
    struct mm_segment_via
    {
	MM_BUFFER_TYPE type;
	void *descriptor_ptr;
	int num_descriptors;
    } via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    struct mm_segment_via_rdma
    {
	MM_BUFFER_TYPE type;
	void *descriptor_ptr;
	int num_descriptors;
    } via_rdma;
#endif
#ifdef WITH_METHOD_NEW
    struct mm_segment_new
    {
	MM_BUFFER_TYPE type;
    }
#endif
} MM_Segment_buffer;

typedef struct MM_Car_msg_header
{
    MPID_Packet pkt;
    MM_Segment_buffer buf;
} MM_Car_msg_header;

typedef struct MM_Car_data_unpacker
{
    union mm_car_data_unpacker_buf
    {
	struct car_unpacker_simple
	{
	    int first;
	    int last;
	} simple;
	struct car_unpacker_tmp
	{
	    int first;
	    int last;
	} tmp;
	/* unpacker method never reads
	struct car_unpacker_vec_read
	{
	} vec_read;
	*/
	struct car_unpacker_vec_write
	{
	    int num_read_copy;
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int vec_size;
	    int total_num_written;
	    int cur_num_written;
	    int cur_index;
	    int num_written_at_cur_index;
	} vec_write;
#ifdef WITH_METHOD_SHM
	struct car_unpacker_shm
	{
	    int num_read;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_unpacker_via
	{
	    int num_read;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_unpacker_via_rdma
	{
	    int num_read;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_unpacker;

typedef union MM_Car_data 
{
    struct car_packer_data
    {
	int first;
	int last;
    } packer;
    MM_Car_data_unpacker unpacker;
#ifdef WITH_METHOD_SHM
    MM_Car_data_shm shm;
#endif
#ifdef WITH_METHOD_TCP
    MM_Car_data_tcp tcp;
#endif
#ifdef WITH_METHOD_SOCKET
    MM_Car_data_socket socket;
#endif
#ifdef WITH_METHOD_VIA
    MM_Car_data_via via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
    MM_Car_data_via_rdma via_rdma;
#endif
#ifdef WITH_METHOD_NEW
    MM_Car_data_new new;
#endif
} MM_Car_data;

/* Communication agent/action request */
typedef struct MM_Car
{
    int freeme;
    struct MPID_Request *request_ptr;
    union MM_Segment_buffer *buf_ptr;
    struct MPIDI_VC *vc_ptr;
    int src, dest;
    MM_CAR_TYPE type;
    MM_Car_data data;
    MM_Car_msg_header msg_header;
    struct MM_Car *next_ptr, *opnext_ptr, *qnext_ptr, *vcqnext_ptr;
} MM_Car;

/* multi-method segment */
typedef struct MM_Segment
{
    int tag;
    union user_buf {
	const void *send;
	void *recv;
    } user_buf;
    int count;
    MPI_Datatype dtype;
    int size;
    int first;
    int last;
    MPID_Segment segment;
    MM_Car *write_list;
    MM_Car wcar[2];
    MM_Car rcar[2];
    int op_valid;
    int (*get_buffers)(struct MPID_Request *request_ptr);
    int (*release_buffers)(struct MPID_Request *request_ptr);
    MM_Segment_buffer buf;
    struct MPID_Request *next_ptr;
} MM_Segment;

#define MPID_DEV_REQUEST_DECL MM_Segment mm;

/* multi-method communicator data */
typedef struct MM_Comm_struct
{
    char *pmi_kvsname;
} MM_Comm_struct;

#define MPID_DEV_COMM_DECL MM_Comm_struct mm;

#define MPID_STATE_LIST_MPID \
MPID_STATE_MM_OPEN_PORT, \
MPID_STATE_MM_CLOSE_PORT, \
MPID_STATE_MM_ACCEPT, \
MPID_STATE_MM_CONNECT, \
MPID_STATE_MM_SEND, \
MPID_STATE_MM_RECV, \
MPID_STATE_MM_CLOSE, \
MPID_STATE_MM_REQUEST_ALLOC, \
MPID_STATE_MM_REQUEST_FREE, \
MPID_STATE_MM_CAR_INIT, \
MPID_STATE_MM_CAR_FINALIZE, \
MPID_STATE_MM_CAR_ALLOC, \
MPID_STATE_MM_CAR_FREE, \
MPID_STATE_MM_VC_INIT, \
MPID_STATE_MM_VC_FINALIZE, \
MPID_STATE_MM_VC_FROM_COMMUNICATOR, \
MPID_STATE_MM_VC_FROM_CONTEXT, \
MPID_STATE_MM_VC_ALLOC, \
MPID_STATE_MM_VC_CONNECT_ALLOC, \
MPID_STATE_MM_VC_FREE, \
MPID_STATE_MM_CHOOSE_BUFFER, \
MPID_STATE_MM_RESET_CARS, \
MPID_STATE_MM_GET_BUFFERS_TMP, \
MPID_STATE_MM_RELEASE_BUFFERS_TMP, \
MPID_STATE_MM_GET_BUFFERS_VEC, \
MPID_STATE_VEC_BUFFER_INIT, \
MPID_STATE_TMP_BUFFER_INIT, \
MPID_STATE_SIMPLE_BUFFER_INIT, \
MPID_STATE_MM_POST_RECV, \
MPID_STATE_MM_POST_SEND, \
MPID_STATE_MM_POST_RNDV_DATA_SEND, \
MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND, \
MPID_STATE_MM_CQ_TEST, \
MPID_STATE_MM_CQ_WAIT, \
MPID_STATE_MM_CQ_ENQUEUE, \
MPID_STATE_MM_CREATE_POST_UNEX, \
MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND, \
MPID_STATE_XFER_INIT, \
MPID_STATE_XFER_RECV_OP, \
MPID_STATE_XFER_RECV_MOP_OP, \
MPID_STATE_XFER_RECV_FORWARD_OP, \
MPID_STATE_XFER_RECV_MOP_FORWARD_OP, \
MPID_STATE_XFER_FORWARD_OP, \
MPID_STATE_XFER_SEND_OP, \
MPID_STATE_XFER_REPLICATE_OP, \
MPID_STATE_XFER_START, \
MPID_STATE_TCP_INIT, \
MPID_STATE_TCP_FINALIZE, \
MPID_STATE_TCP_ACCEPT_CONNECTION, \
MPID_STATE_TCP_GET_BUSINESS_CARD, \
MPID_STATE_TCP_CAN_CONNECT, \
MPID_STATE_TCP_POST_CONNECT, \
MPID_STATE_TCP_POST_READ, \
MPID_STATE_TCP_MERGE_WITH_UNEXPECTED, \
MPID_STATE_TCP_POST_WRITE, \
MPID_STATE_TCP_MAKE_PROGRESS, \
MPID_STATE_TCP_CAR_ENQUEUE, \
MPID_STATE_TCP_CAR_DEQUEUE, \
MPID_STATE_TCP_CAR_DEQUEUE_WRITE, \
MPID_STATE_TCP_RESET_CAR, \
MPID_STATE_TCP_POST_READ_PKT, \
MPID_STATE_TCP_READ, \
MPID_STATE_TCP_WRITE, \
MPID_STATE_TCP_READ_SHM, \
MPID_STATE_TCP_READ_VIA, \
MPID_STATE_TCP_READ_VIA_RDMA, \
MPID_STATE_TCP_READ_VEC, \
MPID_STATE_TCP_READ_TMP, \
MPID_STATE_TCP_READ_CONNECTING, \
MPID_STATE_TCP_WRITE_SHM, \
MPID_STATE_TCP_WRITE_VIA, \
MPID_STATE_TCP_WRITE_VIA_RDMA, \
MPID_STATE_TCP_WRITE_VEC, \
MPID_STATE_TCP_WRITE_TMP, \
MPID_STATE_TCP_STUFF_VECTOR_SHM, \
MPID_STATE_TCP_STUFF_VECTOR_VIA, \
MPID_STATE_TCP_STUFF_VECTOR_VIA_RDMA, \
MPID_STATE_TCP_STUFF_VECTOR_VEC, \
MPID_STATE_TCP_STUFF_VECTOR_TMP, \
MPID_STATE_TCP_WRITE_AGGRESSIVE, \
MPID_STATE_TCP_CAR_HEAD_ENQUEUE, \
MPID_STATE_TCP_SETUP_PACKET_CAR, \
MPID_STATE_TCP_UPDATE_CAR_NUM_WRITTEN, \
MPID_STATE_TCP_MERGE_UNEXPECTED_DATA, \
MPID_STATE_TCP_MERGE_SHM, \
MPID_STATE_TCP_MERGE_VIA, \
MPID_STATE_TCP_MERGE_VIA_RDMA, \
MPID_STATE_TCP_MERGE_VEC, \
MPID_STATE_TCP_MERGE_TMP, \
MPID_STATE_TCP_MERGE_SIMPLE, \
MPID_STATE_TCP_MERGE_WITH_POSTED, \
MPID_STATE_TCP_READ_HEADER, \
MPID_STATE_TCP_READ_DATA, \
MPID_STATE_TCP_READ_SIMPLE, \
MPID_STATE_TCP_WRITE_SIMPLE, \
MPID_STATE_TCP_STUFF_VECTOR_SIMPLE, \
MPID_STATE_MPID_ISEND, \
MPID_STATE_MPID_IRECV, \
MPID_STATE_MPID_SEND, \
MPID_STATE_MPID_RECV, \
MPID_STATE_MPID_PROGRESS_TEST, \
MPID_STATE_MPID_ABORT, \
MPID_STATE_MPID_CLOSE_PORT, \
MPID_STATE_MPID_COMM_ACCEPT, \
MPID_STATE_MPID_COMM_CONNECT, \
MPID_STATE_MPID_COMM_DISCONNECT, \
MPID_STATE_MPID_COMM_SPAWN_MULTIPLE, \
MPID_STATE_MPID_OPEN_PORT, \
MPID_STATE_FIND_IN_QUEUE, \
MPID_STATE_CQ_HANDLE_READ_HEAD_CAR, \
MPID_STATE_CQ_HANDLE_READ_DATA_CAR, \
MPID_STATE_CQ_HANDLE_READ_CAR, \
MPID_STATE_CQ_HANDLE_WRITE_HEAD_CAR, \
MPID_STATE_CQ_HANDLE_WRITE_DATA_CAR, \
MPID_STATE_CQ_HANDLE_WRITE_CAR, \
MPID_STATE_PACKER_CAR_ENQUEUE, \
MPID_STATE_PACKER_CAR_DEQUEUE, \
MPID_STATE_PACKER_INIT, \
MPID_STATE_PACKER_FINALIZE, \
MPID_STATE_PACKER_MAKE_PROGRESS, \
MPID_STATE_PACKER_POST_READ, \
MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED, \
MPID_STATE_PACKER_POST_WRITE, \
MPID_STATE_PACKER_RESET_CAR, \
MPID_STATE_MPID_PROGRESS_WAIT, \
MPID_STATE_MPID_REQUEST_RELEASE, \
MPID_STATE_SHM_CAN_CONNECT, \
MPID_STATE_SHM_GET_BUSINESS_CARD, \
MPID_STATE_SHM_INIT, \
MPID_STATE_SHM_FINALIZE, \
MPID_STATE_SHM_MAKE_PROGRESS, \
MPID_STATE_SHM_ALLOC, \
MPID_STATE_SHM_FREE, \
MPID_STATE_SHM_GET_MEM_SYNC, \
MPID_STATE_SHM_RELEASE_MEM, \
MPID_STATE_SHM_POST_CONNECT, \
MPID_STATE_SHM_POST_READ, \
MPID_STATE_SHM_MERGE_WITH_UNEXPECTED, \
MPID_STATE_SHM_POST_WRITE, \
MPID_STATE_UNPACKER_CAR_ENQUEUE, \
MPID_STATE_UNPACKER_CAR_DEQUEUE, \
MPID_STATE_UNPACKER_INIT, \
MPID_STATE_UNPACKER_FINALIZE, \
MPID_STATE_UNPACKER_MAKE_PROGRESS, \
MPID_STATE_UNPACKER_WRITE_SHM, \
MPID_STATE_UNPACKER_WRITE_VIA, \
MPID_STATE_UNPACKER_WRITE_VIA_RDMA, \
MPID_STATE_UNPACKER_WRITE_VEC, \
MPID_STATE_UNPACKER_WRITE_TMP, \
MPID_STATE_UNPACKER_POST_READ, \
MPID_STATE_UNPACKER_MERGE_WITH_UNEXPECTED, \
MPID_STATE_UNPACKER_POST_WRITE, \
MPID_STATE_UNPACKER_RESET_CAR, \
MPID_STATE_UNPACKER_WRITE_SIMPLE, \
MPID_STATE_VIA_CAN_CONNECT, \
MPID_STATE_VIA_GET_BUSINESS_CARD, \
MPID_STATE_VIA_INIT, \
MPID_STATE_VIA_FINALIZE, \
MPID_STATE_VIA_MAKE_PROGRESS, \
MPID_STATE_VIA_POST_CONNECT, \
MPID_STATE_VIA_POST_READ, \
MPID_STATE_VIA_MERGE_WITH_UNEXPECTED, \
MPID_STATE_VIA_POST_WRITE, \
MPID_STATE_VIA_RDMA_CAN_CONNECT, \
MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD, \
MPID_STATE_VIA_RDMA_INIT, \
MPID_STATE_VIA_RDMA_FINALIZE, \
MPID_STATE_VIA_RDMA_MAKE_PROGRESS, \
MPID_STATE_VIA_RDMA_POST_CONNECT, \
MPID_STATE_VIA_RDMA_POST_READ, \
MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED, \
MPID_STATE_VIA_RDMA_POST_WRITE, \
MPID_STATE_COPYSET, \

#define MPID_STATE_LIST_BSOCKET \
MPID_STATE_BACCEPT, \
MPID_STATE_BBIND, \
MPID_STATE_BCLOSE, \
MPID_STATE_BCLR, \
MPID_STATE_BCONNECT, \
MPID_STATE_BCOPYSET, \
MPID_STATE_BEASY_ACCEPT, \
MPID_STATE_BEASY_CLOSESOCKET, \
MPID_STATE_BEASY_CONNECT, \
MPID_STATE_BEASY_CREATE, \
MPID_STATE_BEASY_GET_IP, \
MPID_STATE_BEASY_GET_IP_STRING, \
MPID_STATE_BEASY_GET_SOCK_INFO, \
MPID_STATE_BEASY_RECEIVE, \
MPID_STATE_BEASY_RECEIVE_SOME, \
MPID_STATE_BEASY_RECEIVE_TIMEOUT, \
MPID_STATE_BEASY_SEND, \
MPID_STATE_BGETSOCKNAME, \
MPID_STATE_BLISTEN, \
MPID_STATE_BMAKE_BLOCKING, \
MPID_STATE_BMAKE_NONBLOCKING, \
MPID_STATE_BREAD, \
MPID_STATE_BREADV, \
MPID_STATE_BSELECT, \
MPID_STATE_BSET, \
MPID_STATE_BSETSOCKOPT, \
MPID_STATE_BSOCKET, \
MPID_STATE_BWRITE, \
MPID_STATE_BWRITEV, \
MPID_STATE_READ, \
MPID_STATE_READV, \
MPID_STATE_SELECT, \

#endif
