/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"
#ifdef HAVE_ERRNO_H
#include "errno.h"
#endif

int MM_Recv(int conn, char *buffer, int length)
{
    int error;
    if (beasy_receive(conn, buffer, length) != SOCKET_ERROR)
	return length;
    error = beasy_getlasterror();
    printf("beasy_receive failed, error %d\n", error);
    return SOCKET_ERROR;
}

