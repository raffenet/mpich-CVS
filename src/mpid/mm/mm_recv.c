/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsocket.h"

int MM_Recv(int conn, char *buffer, int length)
{
    int error;
    if (beasy_receive(conn, buffer, length) != SOCKET_ERROR)
	return length;
    error = WSAGetLastError();
    printf("beasy_receive failed, error %d\n", error);
    return SOCKET_ERROR;
}

