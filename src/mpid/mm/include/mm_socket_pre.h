/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_SOCKET_PRE_H
#define MM_SOCKET_PRE_H

#define SOCKET_INVALID_STATE   0x00
#define SOCKET_CONNECTING      0x01
#define SOCKET_ACCEPTING       0x02
#define SOCKET_CONNECT_MASK    0x03
#define SOCKET_CONNECTED       0x04
#define SOCKET_READING_HEADER  0x08
#define SOCKET_READING_DATA    0x10
#define SOCKET_WRITING_HEADER  0x20
#define SOCKET_WRITING_DATA    0x40

#define SOCKET_READING_ACK          0x01
#define SOCKET_WRITING_ACK          0x02
#define SOCKET_READING_CONNECT_PKT  0x04
#define SOCKET_WRITING_CONNECT_PKT  0x08
#define SOCKET_POSTING_CONNECT      0x10

#define SOCKET_SET_BIT(a,b)         a |= ( b )
#define SOCKET_CLR_BIT(a,b)         a &= ~( b )
#define SOCKET_CLR_SET_BITS(a,b,c)  a = ( a & ~( b )) | ( c )

typedef struct MM_Car_data_socket
{
    union mm_car_data_socket_buf
    {
	struct car_socket_simple
	{
	    int num_written;
	} simple;
	struct car_socket_tmp
	{
	    int num_written;
	} tmp;
	struct car_socket_vec_read
	{
	    MPID_IOV vec[MPID_IOV_LIMIT];
	    int vec_size;
	    int total_num_read;
	    int cur_num_read;
	    int cur_index;
	} vec_read;
	struct car_socket_vec_write
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
	struct car_socket_shm
	{
	    int num_written;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_socket_via
	{
	    int num_written;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_socket_via_rdma
	{
	    int num_written;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_socket;

#endif
