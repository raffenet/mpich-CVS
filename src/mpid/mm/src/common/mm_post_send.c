/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int mm_post_send(MM_Car *car_ptr)
{
    MM_ENTER_FUNC(MM_POST_SEND);

    dbg_printf("mm_post_send\n");
    car_ptr->vc_ptr->post_write(car_ptr->vc_ptr, car_ptr);

    MM_EXIT_FUNC(MM_POST_SEND);
    return MPI_SUCCESS;
}
