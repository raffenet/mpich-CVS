/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   mm_reset_cars - reset cars

   Parameters:
+  MPID_Request *request_ptr - request

   Notes:
@*/
int mm_reset_cars(MPID_Request *request_ptr)
{
    MM_Car *car_ptr;

    car_ptr = &request_ptr->mm.rcar[0];
    while (car_ptr)
    {
	if (car_ptr->vc_ptr)
	{
	    car_ptr->vc_ptr->reset_car(car_ptr);
	}
	else
	{
	    dbg_printf("reset_car not available, no vc_ptr\n");
	}
	car_ptr = car_ptr->opnext_ptr;
    }

    car_ptr = &request_ptr->mm.wcar[0];
    while (car_ptr)
    {
	if (car_ptr->vc_ptr)
	{
	    car_ptr->vc_ptr->reset_car(car_ptr);
	}
	else
	{
	    dbg_printf("reset_car not available, no vc_ptr\n");
	}
	car_ptr = car_ptr->opnext_ptr;
    }

    return MPI_SUCCESS;
}
