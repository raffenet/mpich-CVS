/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "smpd.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int main(int argc, char* argv[])
{
    int result;
    sock_set_t set, new_set;
    sock_t listener, new_sock;
    sock_event_t event;
    int port = SMPD_LISTENER_PORT;

    smpd_enter_fn("main");

#ifdef HAVE_WINDOWS_H
    /* prevent the os from bringing up debug message boxes if this process crashes */
    /*SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);*/
#endif

    /* initialization */
    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_init_process failed.\n");
	smpd_exit_fn("main");
	return result;
    }

    /* parse the command line */
    result = smpd_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the command arguments.\n");
	smpd_exit_fn("main");
	return result;
    }

    /* This process is the root_smpd.  All sessions are child processes of this process. */
    smpd_process.id = 0;
    smpd_process.root_smpd = SMPD_TRUE;

    /*smpd_set_smpd_data("path", smpd_process.pszExe);*/

    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_create_set failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }
    smpd_dbg_printf("created a set for the listener: %d\n", sock_getsetid(set));
    result = sock_listen(set, NULL, &port, &listener); 
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_listen failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }
    smpd_dbg_printf("smpd listening on port %d\n", port);

    while (1)
    {
	smpd_dbg_printf("smpd main calling sock_wait.\n");
	event.error = SOCK_SUCCESS;
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    if (result == SOCK_ERR_TIMEOUT)
	    {
		smpd_err_printf("Warning: sock_wait returned SOCK_ERR_TIMEOUT when infinite time was passed in.\n");
		continue;
	    }
	    if (event.error != SOCK_SUCCESS)
		result = event.error;
	    smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    sock_finalize();
	    smpd_exit_fn("main");
	    return result;
	}
	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    smpd_err_printf("sock_wait returned SOCK_OP_READ unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("error: %s\n", get_sock_error_string(event.error));
	    break;
	case SOCK_OP_WRITE:
	    smpd_err_printf("sock_wait returned SOCK_OP_WRITE unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("error: %s\n", get_sock_error_string(event.error));
	    break;
	case SOCK_OP_ACCEPT:
	    if (event.error != SOCK_SUCCESS)
	    {
		smpd_err_printf("error listening and accepting socket: %s\n", get_sock_error_string(event.error));
		break;
	    }
	    result = sock_create_set(&new_set);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("error creating a new set for the newly accepted socket:\n%s\n", get_sock_error_string(result));
		break;
	    }
	    smpd_dbg_printf("accepting new socket into new set: %d\n", sock_getsetid(new_set));
	    result = sock_accept(listener, new_set, NULL, &new_sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
		break;
	    }
	    smpd_dbg_printf("authenticating new connection\n");
	    result = smpd_authenticate(new_set, new_sock, SMPD_SERVER_AUTHENTICATION);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("server connection authentication failed.\n");
		smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(new_sock));
		result = sock_post_close(new_sock);
		if (result != SOCK_SUCCESS)
		{
		    smpd_err_printf("unable to close the rejected socket.\n");
		}
		break;
	    }
	    smpd_dbg_printf("starting manager\n");
	    result = smpd_start_mgr(new_set, new_sock);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("error starting management session.\n");
	    }
	    break;
	case SOCK_OP_CONNECT:
	    smpd_err_printf("sock_wait returned SOCK_OP_CONNECT unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("error: %s\n", get_sock_error_string(event.error));
	    break;
	case SOCK_OP_CLOSE:
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("error closing socket: %s\n", get_sock_error_string(event.error));
	    if (event.user_ptr != NULL)
	    {
	    }
	    break;
	default:
	    break;
	}
    }

    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }
    smpd_exit_fn("main");
    return 0;
}

