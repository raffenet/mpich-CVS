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
    MM_ENTER_FUNC(MPID_PROGRESS_TEST);

    /* test the completion queue */
    mm_cq_test();

    MM_EXIT_FUNC(MPID_PROGRESS_TEST);
    return 0;
}
