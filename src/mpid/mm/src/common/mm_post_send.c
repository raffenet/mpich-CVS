/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_send(MM_Car *car_ptr)
{
    /* check if the car should be placed in one of the packer unpacker lists */
    if (car_ptr->type & MM_PACKER_CAR)
    {
	car_ptr->qnext_ptr = MPID_Process.pkr_write_list;
	MPID_Process.pkr_write_list = car_ptr;
	return MPI_SUCCESS;
    }
    if (car_ptr->type & MM_UNPACKER_CAR)
    {
	car_ptr->qnext_ptr = MPID_Process.unpkr_write_list;
	MPID_Process.unpkr_write_list = car_ptr;
	return MPI_SUCCESS;
    }
    /* else call the method post_write function in the vc */
    car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr);
    return MPI_SUCCESS;
}
