/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

#ifdef WITH_METHOD_SHM
int tcp_stuff_vector_shm(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_SHM);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_SHM);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA
int tcp_stuff_vector_via(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_VIA);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_VIA);
    return MPI_SUCCESS;
}
#endif

#ifdef WITH_METHOD_VIA_RDMA
int tcp_stuff_vector_via_rdma(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_VIA_RDMA);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_VIA_RDMA);
    return MPI_SUCCESS;
}
#endif

int tcp_stuff_vector_vec(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_VEC);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_VEC);
    return MPI_SUCCESS;
}

int tcp_stuff_vector_tmp(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_TMP);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_TMP);
    return MPI_SUCCESS;
}

#ifdef WITH_METHOD_NEW
int tcp_stuff_vector_new(MPID_VECTOR *vec, int *cur_pos, MM_Car *car_ptr, MM_Segment_buffer *buf_ptr)
{
    MM_ENTER_FUNC(TCP_STUFF_VECTOR_NEW);
    MM_EXIT_FUNC(TCP_STUFF_VECTOR_NEW);
    return MPI_SUCCESS;
}
#endif

int tcp_write_aggressive(MPIDI_VC *vc_ptr)
{
    MM_Car *car_ptr;
    MM_Segment_buffer *buf_ptr;
    MPID_VECTOR vec[MPID_VECTOR_LIMIT];
    int cur_pos = 0;
    BOOL stop = FALSE;

    MM_ENTER_FUNC(TCP_WRITE_AGGRESSIVE);

    if (!vc_ptr->data.tcp.connected)
    {
	MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
	return MPI_SUCCESS;
    }

    if (vc_ptr->writeq_head == NULL)
    {
	MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
	return MPI_SUCCESS;
    }

    car_ptr = vc_ptr->writeq_head;

    /* pack as many cars into a vector as possible */
    while (car_ptr)
    {
	buf_ptr = car_ptr->buf_ptr;
	switch (buf_ptr->type)
	{
#ifdef WITH_METHOD_SHM
	case MM_SHM_BUFFER:
	    stop = !tcp_stuff_vector_shm(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA
	case MM_VIA_BUFFER:
	    stop = !tcp_stuff_vector_via(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	case MM_VIA_RDMA_BUFFER:
	    stop = !tcp_stuff_vector_via_rdma(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
	case MM_VEC_BUFFER:
	    if (buf_ptr->vec.num_cars_outstanding > 0)
	    {
		stop = !tcp_stuff_vector_vec(vec, &cur_pos, car_ptr, buf_ptr);
	    }
	    else
		stop = TRUE;
	    break;
	case MM_TMP_BUFFER:
	    stop = !tcp_stuff_vector_tmp(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#ifdef WITH_METHOD_NEW
	case MM_NEW_METHOD_BUFFER:
	    stop = !tcp_stuff_vector_new(vec, &cur_pos, car_ptr, buf_ptr);
	    break;
#endif
	case MM_NULL_BUFFER:
	    err_printf("Error: tcp_write_aggressive called on a null buffer\n");
	    break;
	default:
	    err_printf("Error: tcp_write_aggressive: unknown or unsupported buffer type: %d\n", buf_ptr->type);
	    break;
	}
	if (stop)
	    break;
	car_ptr = car_ptr->next_ptr;
    }

    /* write the data */

    /* update all the cars and buffers affected */

    MM_EXIT_FUNC(TCP_WRITE_AGGRESSIVE);
    return MPI_SUCCESS;
}
