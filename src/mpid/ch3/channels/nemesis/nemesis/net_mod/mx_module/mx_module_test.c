/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_module_impl.h"
#include "myriexpress.h"

int
MPID_nem_mx_module_test()
{
    return 0; //gm_receive_pending (MPID_nem_module_gm_port) || !MPID_nem_queue_empty (MPID_nem_module_gm_recv_queue);
}
