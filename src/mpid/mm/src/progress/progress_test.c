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
    MPID_STATE_DECLS;
    MPID_FUNC_ENTER(MPID_STATE_MPID_PROGRESS_TEST);

    /* test the completion queue */
    mm_cq_test();

    MPID_FUNC_EXIT(MPID_STATE_MPID_PROGRESS_TEST);
    return 0;
}
