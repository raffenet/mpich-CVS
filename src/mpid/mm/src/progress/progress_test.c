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
    /* advance any read packing */
    if (MPID_Process.pkr_read_list)
	mm_packer_read();

    /* test the completion queue */
    mm_cq_test();

    /* advance any write packing and unpacking */
    if (MPID_Process.pkr_write_list)
	mm_packer_write();
    if (MPID_Process.unpkr_write_list)
	mm_unpacker_write();

    return 0;
}
