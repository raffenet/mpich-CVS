/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_get_business_card(char *value, int length)
{
    MM_ENTER_FUNC(TCP_GET_BUSINESS_CARD);

    snprintf(value, length, "%s:%d", TCP_Process.host, TCP_Process.port);

    MM_EXIT_FUNC(TCP_GET_BUSINESS_CARD);
    return MPI_SUCCESS;
}
