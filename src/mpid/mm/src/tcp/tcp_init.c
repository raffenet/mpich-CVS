/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

TCP_PerProcess TCP_Process;

/*@
   tcp_init - initialize the tcp method


   Notes:
@*/
int tcp_init()
{
    int err;
    char str[256];

    err = beasy_create(&TCP_Process.listener, ADDR_ANY, INADDR_ANY);
    if (err == SOCKET_ERROR)
    {
	err = beasy_getlasterror();
	beasy_error_to_string(err, str, 256);
	err_printf("tcp_init: unable to create the listener socket\nError: %s\n", str);
    }
    err = blisten(TCP_Process.listener, 10);
    if (err == SOCKET_ERROR)
    {
	err = beasy_getlasterror();
	beasy_error_to_string(err, str, 256);
	err_printf("tcp_init: unable to listen on the listener socket\nError: %s\n", str);
    }
    err = beasy_get_sock_info(TCP_Process.listener, TCP_Process.host, &TCP_Process.port);
    if (err == SOCKET_ERROR)
    {
	err = beasy_getlasterror();
	beasy_error_to_string(err, str, 256);
	err_printf("tcp_init: unable to get the hostname and port of the listener socket\nError: %s\n", str);
    }

    BFD_ZERO(&TCP_Process.writeset);
    BFD_ZERO(&TCP_Process.readset);
    BFD_SET(TCP_Process.listener, &TCP_Process.readset);

    return MPI_SUCCESS;
}

/*@
   tcp_finalize - finalize the tcp method


   Notes:
@*/
int tcp_finalize()
{
    beasy_closesocket(TCP_Process.listener);
    TCP_Process.listener = BFD_INVALID_SOCKET;

    return MPI_SUCCESS;
}
