/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ibimpl.h"

IB_PerProcess IB_Process;

/*@
   ib_init - initialize the ib method

   Notes:
@*/
int ib_init()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_INIT);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_INIT);
    return MPI_SUCCESS;
}

/*@
   ib_finalize - finalize the ib method

   Notes:
@*/
int ib_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IB_FINALIZE);
    MPIDI_FUNC_ENTER(MPID_STATE_IB_FINALIZE);

    MPIDI_FUNC_EXIT(MPID_STATE_IB_FINALIZE);
    return MPI_SUCCESS;
}

void ib_format_sock_error(int error)
{
}

void ib_print_sock_error(int error, char *msg)
{
}
