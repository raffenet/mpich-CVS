/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

int socket_get_business_card(char *value, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_GET_BUSINESS_CARD);
    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_GET_BUSINESS_CARD);

    snprintf(value, length, "%s:%d", SOCKET_Process.host, SOCKET_Process.port);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
