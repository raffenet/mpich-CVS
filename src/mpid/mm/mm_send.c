/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsocket.h"

int MM_Send(int conn, char *buffer, int length)
{
    int error;
    if (beasy_send(conn, buffer, length) != SOCKET_ERROR)
	return length;
#ifdef HAVE_WINDOWS_H
    error = WSAGetLastError();
#else
    error = errno;
#endif
    printf("beasy_send failed, error %d\n", error);
    return SOCKET_ERROR;
}

