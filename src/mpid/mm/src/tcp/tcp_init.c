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
    int error;

    MM_ENTER_FUNC(TCP_INIT);

    error = bsocket_init();
    if (error)
    {
	err_printf("tcp_init: bsocket_init failed, error %d\n", error);
    }
    if (beasy_create(&TCP_Process.listener, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_init: unable to create the listener socket\nError: %s\n", TCP_Process.err_msg);
    }
    if (blisten(TCP_Process.listener, 10) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_init: unable to listen on the listener socket\nError: %s\n", TCP_Process.err_msg);
    }
    if (beasy_get_sock_info(TCP_Process.listener, TCP_Process.host, &TCP_Process.port) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_init: unable to get the hostname and port of the listener socket\nError: %s\n", TCP_Process.err_msg);
    }

    BFD_ZERO(&TCP_Process.writeset);
    BFD_ZERO(&TCP_Process.readset);
    BFD_SET(TCP_Process.listener, &TCP_Process.readset);
    TCP_Process.max_bfd = TCP_Process.listener;

    MM_EXIT_FUNC(TCP_INIT);
    return MPI_SUCCESS;
}

/*@
   tcp_finalize - finalize the tcp method

   Notes:
@*/
int tcp_finalize()
{
    MM_ENTER_FUNC(TCP_FINALIZE);

    beasy_closesocket(TCP_Process.listener);
    TCP_Process.listener = BFD_INVALID_SOCKET;

    bsocket_finalize();

    MM_EXIT_FUNC(TCP_FINALIZE);
    return MPI_SUCCESS;
}
