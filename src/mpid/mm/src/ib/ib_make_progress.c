/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

#ifdef WITH_METHOD_IB

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

int ib_handle_accept()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_HANDLE_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_HANDLE_ACCEPT);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_HANDLE_ACCEPT);
    return MPI_SUCCESS;
}

/*@
   ib_make_progress - make progress

   Notes:
@*/
int ib_make_progress()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_MAKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MAKE_PROGRESS);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MAKE_PROGRESS);
    return MPI_SUCCESS;
}

#endif
