/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_TCP_PRE_H
#define MM_TCP_PRE_H

typedef struct MM_Car_data_tcp
{
    union mm_car_data_tcp_buf
    {
	struct car_tcp_tmp
	{
	    int num_read;
	} tmp;
	struct car_tcp_vec
	{
	    MPID_VECTOR vec[MPID_VECTOR_LIMIT];
	    int len;
	} vec;
#ifdef WITH_METHOD_SHM
	struct car_tcp_shm
	{
	    int num_read;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_tcp_via
	{
	    int num_read;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_tcp_via_rdma
	{
	    int num_read;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_tcp;

#endif
