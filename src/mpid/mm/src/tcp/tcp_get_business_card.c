/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_get_business_card(char *value, int length)
{
    MPID_STATE_DECL(MPID_STATE_TCP_GET_BUSINESS_CARD);
    MPID_FUNC_ENTER(MPID_STATE_TCP_GET_BUSINESS_CARD);

    snprintf(value, length, "%s:%d", TCP_Process.host, TCP_Process.port);

    MPID_FUNC_EXIT(MPID_STATE_TCP_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
