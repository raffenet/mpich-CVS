/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_recv(int conn, char *buffer, int length)
{
    int error;
    MPID_STATE_DECL(MPID_STATE_MM_RECV);
    MPID_FUNC_ENTER(MPID_STATE_MM_RECV);

    if (beasy_receive(conn, buffer, length) != SOCKET_ERROR)
    {
	MPID_FUNC_EXIT(MPID_STATE_MM_RECV);
	return length;
    }
    error = beasy_getlasterror();
    err_printf("beasy_receive failed, error %d\n", error);

    MPID_FUNC_EXIT(MPID_STATE_MM_RECV);
    return SOCKET_ERROR;
}

