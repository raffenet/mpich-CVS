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
#include "mpiimpl.h"

#ifdef WITH_METHOD_TCP
#include "mm_tcp_pre.h"
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
#define MPID_VECTOR         WSABUF
#define MPID_VECTOR_LEN     len
#define MPID_VECTOR_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_VECTOR         struct iovec
#define MPID_VECTOR_LEN     iov_len
#define MPID_VECTOR_BUF     iov_base
#endif
#define MPID_VECTOR_LIMIT   16

/* Buffer type */
typedef enum MM_BUFFER_TYPE {
    MM_NULL_BUFFER,
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
    MPID_RNDV_OK_TO_SEND_PKT,
    MPID_RNDV_DATA_PKT,
    MPID_RDMA_ACK_PKT,
    MPID_RDMA_DATA_ACK_PKT,
    MPID_RDMA_REQUEST_DATA_ACK_PKT
} MPID_Packet_type;

/* Communication agent request type */
typedef int MM_CAR_TYPE;
#define MM_NULL_CAR      0x00
#define MM_HEAD_CAR      0x01
#define MM_READ_CAR      0x02
#define MM_WRITE_CAR     0x04
#define MM_PACKER_CAR    0x08
#define MM_UNPACKER_CAR  0x10
#define MM_UNEX_HEAD_CAR 0x20
#define MM_UNEX_CAR      0x40

/* packet definitions */
typedef struct MPID_Packet
{
    int type;
    int context;
    int tag;
    int src;
    int size;
} MPID_Packet;

typedef union MM_Car_data 
{
    MPID_Packet pkt;
    struct car_packer
    {
	int first;
	int last;
    } packer;
    struct car_unpacker
    {
	int first;
	int last;
    } unpacker;
#ifdef WITH_METHOD_SHM
    MM_Car_data_shm shm;
#endif
#ifdef WITH_METHOD_TCP
    MM_Car_data_tcp tcp;
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

typedef union MM_Segment_buffer
{
    MM_BUFFER_TYPE type;
    struct mm_segment_tmp
    {
	MM_BUFFER_TYPE type;
	void *buf_ptr[2];
	int cur_buf;
	int num_read;
	int min_num_written;
    } tmp;
    struct mm_segment_vec
    {
	MM_BUFFER_TYPE type;
	MPID_VECTOR vec[MPID_VECTOR_LIMIT];
	int vec_size;
	int msg_size;
	int num_read;
	int first, last;
	int min_num_written;
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
    struct MM_Car *next_ptr, *opnext_ptr, *qnext_ptr, *mnext_ptr;
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
    MM_Segment_buffer buf, pkt_buf;
    struct MPID_Request *next_ptr;
} MM_Segment;

#define MPID_DEV_REQUEST_DECL MM_Segment mm;

/* multi-method communicator data */
typedef struct MM_Comm_struct
{
    char *pmi_kvsname;
} MM_Comm_struct;

#define MPID_DEV_COMM_DECL MM_Comm_struct mm;

#endif
