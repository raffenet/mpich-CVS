/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "bsocket.h"

int MM_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int bfd;
    int error;
    char host[40];
    int port;
    OpenPortNode_t *p;

    if (beasy_create(&bfd, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	error = beasy_getlasterror();
	printf("beasy_create failed, error %d\n", error);
	return error;
    }
    if (blisten(bfd, 5) == SOCKET_ERROR)
    {
	error = beasy_getlasterror();
	printf("blisten failed, error %d\n", error);
	return error;
    }
    beasy_get_sock_info(bfd, host, &port);
    beasy_get_ip_string(host);

    sprintf(port_name, "%s:%d", host, port);

    p = (OpenPortNode_t*)MPIU_Malloc(sizeof(OpenPortNode_t));
    p->bfd = bfd;
    strncpy(p->port_name, port_name, MPI_MAX_PORT_NAME);
    p->next = MPID_Process.port_list;
    MPID_Process.port_list = p;

    return 0;
}
