/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int mm_accept(MPID_Info *info_ptr, char *port_name)
{
    int error;
    int bfd;
    OpenPortNode *p;

    p = MPID_Process.port_list;
    while (p)
    {
	if (strncmp(p->port_name, port_name, MPI_MAX_PORT_NAME) == 0)
	{
	    bfd = beasy_accept(p->bfd);
	    if (bfd == BFD_INVALID_SOCKET)
	    {
		error = beasy_getlasterror();
		printf("beasy_accept failed, error %d\n", error);
		return BFD_INVALID_SOCKET;
	    }
	    return bfd;
	}
	p = p->next;
    }

    return BFD_INVALID_SOCKET;
}

