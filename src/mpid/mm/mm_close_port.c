/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsocket.h"

int MM_Close_port(char *port_name)
{
    OpenPortNode_t *p, *pTrailer;

    pTrailer = p = MPIR_Process.port_list;
    if (p == NULL)
	return -1;

    while (p)
    {
	if (strcmp(port_name, p->port_name) == 0)
	{
	    beasy_closesocket(p->bfd);
	    if (p == MPIR_Process.port_list)
		MPIR_Process.port_list = MPIR_Process.port_list->next;
	    else
		pTrailer->next = p->next;
	    MPIU_Free(p);
	    return 0;
	}
	if (pTrailer != p)
	    pTrailer = pTrailer->next;
	p = p->next;
    }

    return -1;
}

