/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Progress_wait - wait for an event to end a progress block

   Parameters:

   Notes:
@*/
void MPID_Progress_wait( void )
{
    MM_ENTER_FUNC(MPID_PROGRESS_WAIT);

    MPID_Progress_test();

    MM_EXIT_FUNC(MPID_PROGRESS_WAIT);
}
