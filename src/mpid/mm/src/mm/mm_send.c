/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_send(int conn, char *buffer, int length)
{
    int error;

    MM_ENTER_FUNC(MM_SEND);

    if (beasy_send(conn, buffer, length) != SOCKET_ERROR)
    {
	MM_EXIT_FUNC(MM_SEND);
	return length;
    }
    error = beasy_getlasterror();
    err_printf("beasy_send failed, error %d\n", error);

    MM_EXIT_FUNC(MM_SEND);
    return SOCKET_ERROR;
}

