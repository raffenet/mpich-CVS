/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_get_opt(int argc, char *argv[], char *opt, char *val, int len)
{
    int i;
    for (i=1; i<argc; i++)
    {
	if (strcmp(argv[i], opt) == 0)
	{
	    if (val == NULL || len == 0)
		return 1;
	    if (i+1 < argc)
	    {
		strncpy(val, argv[i+1], len);
		val[len-1] = '\0';
		return 1;
	    }
	    smpd_err_printf("option '%s' matched but no value specified\n", opt);
	}
    }
    return 0;
}

int smpd_parse_command_args(int argc, char *argv[])
{
#ifdef HAVE_WINDOWS_H
    char str[20], handle_str[20];
    int port;
    sock_t listener, session_sock;
    sock_set_t set, session_set;
    sock_event_t event;
    int result;
    HANDLE hPipe;
    DWORD num_written;
#endif

#ifdef HAVE_WINDOWS_H
    if (smpd_get_opt(argc, argv, "-mgr", handle_str, 20))
    {
	result = sock_init();
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	result = sock_create_set(&set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set(listener) failed, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	result = sock_create_set(&session_set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set(session) failed, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	result = sock_listen(set, NULL, &port, &listener); 
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_listen failed, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("smpd manager listening on port %d\n", port);
	hPipe = (HANDLE)atol(handle_str);
	memset(str, 0, 20);
	snprintf(str, 20, "%d", port);
	smpd_dbg_printf("manager writing port string back to smpd.\n");
	if (!WriteFile(hPipe, str, 20, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile failed, error %d\n", GetLastError());
	    return SMPD_FAIL;
	}
	CloseHandle(hPipe);
	if (num_written != 20)
	{
	    smpd_err_printf("wrote only %d bytes of 20\n", num_written);
	    return SMPD_FAIL;
	}
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock failure waiting for re-connection, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	if (event.op_type != SOCK_OP_ACCEPT)
	{
	    smpd_err_printf("unexpected sock operation while waiting for re-connection: op_type = %d\n", event.op_type);
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("accepting reconnection.\n");
	result = sock_accept(listener, session_set, NULL, &session_sock);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_accept failed, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("closing the manager's listener set.\n");
	/* Are we allowed to post a close on the listener?
	result = sock_post_close(listener);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_post_close(listener) failed, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock failure waiting for listener to close, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	if (event.op_type != SOCK_OP_CLOSE)
	{
	    smpd_err_printf("unexpected sock operation while waiting for listener to close: op_type = %d\n", event.op_type);
	    return SMPD_FAIL;
	}
	*/
	result = sock_destroy_set(set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("Unable to destroy the listener set, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	smpd_session(session_set, session_sock);
    }
#endif

    return SMPD_SUCCESS;
}
