/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Progress_test - test and end a progress block

   Parameters:

   Notes:
@*/
int MPID_Progress_test( void )
{
    MPIDI_VC *vc_ptr;

    vc_ptr = MPID_Process.read_list;
    while (vc_ptr)
    {
	vc_ptr->read(vc_ptr);
	vc_ptr = vc_ptr->read_next_ptr;
    }

    mm_cq_test();

    vc_ptr = MPID_Process.write_list;
    while (vc_ptr)
    {
	vc_ptr->write(vc_ptr);
	vc_ptr = vc_ptr->write_next_ptr;
    }

    return 0;
}
