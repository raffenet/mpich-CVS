/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

#ifdef HAVE_WINDOWS_H
char *smpd_encode_handle(char *str, HANDLE h)
{
    sprintf(str, "%p", h);
    return str;
}

HANDLE smpd_decode_handle(char *str)
{
    HANDLE p;
    sscanf(str, "%p", &p);
    return p;
}
#endif

int smpd_parse_command_args(int *argcp, char **argvp[])
{
#ifdef HAVE_WINDOWS_H
    char str[20], read_handle_str[20], write_handle_str[20];
    int port;
    sock_t listener, session_sock;
    sock_set_t set, session_set;
    sock_event_t event;
    int result;
    HANDLE hWrite, hRead;
    DWORD num_written, num_read;
#endif

    smpd_dbg_printf("entering smpd_parse_command_args.\n");

#ifdef HAVE_WINDOWS_H
    if (smpd_get_opt(argcp, argvp, "-mgr"))
    {
	if (!smpd_get_opt_string(argcp, argvp, "-read", read_handle_str, 20))
	{
	    smpd_err_printf("manager started without a read pipe handle.\n");
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	if (!smpd_get_opt_string(argcp, argvp, "-write", write_handle_str, 20))
	{
	    smpd_err_printf("manager started without a write pipe handle.\n");
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	hRead = smpd_decode_handle(read_handle_str);
	hWrite = smpd_decode_handle(write_handle_str);

	smpd_dbg_printf("manager creating listener and session sets.\n");

	result = sock_create_set(&set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set(listener) failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	result = sock_create_set(&session_set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set(session) failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("created set for manager listener, %d, and for the session sockets, %d\n",
	    sock_getsetid(set), sock_getsetid(session_set));
	port = 0;
	result = sock_listen(set, NULL, &port, &listener); 
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_listen failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("smpd manager listening on port %d\n", port);

	memset(str, 0, 20);
	snprintf(str, 20, "%d", port);
	smpd_dbg_printf("manager writing port back to smpd.\n");
	if (!WriteFile(hWrite, str, 20, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile failed, error %d\n", GetLastError());
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	CloseHandle(hWrite);
	if (num_written != 20)
	{
	    smpd_err_printf("wrote only %d bytes of 20\n", num_written);
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("manager reading account and password from smpd.\n");
	if (!ReadFile(hRead, smpd_process.UserAccount, 100, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	if (num_read != 100)
	{
	    smpd_err_printf("read only %d bytes of 100\n", num_read);
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.UserPassword, 100, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	if (num_read != 100)
	{
	    smpd_err_printf("read only %d bytes of 100\n", num_read);
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock failure waiting for re-connection, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	if (event.op_type != SOCK_OP_ACCEPT)
	{
	    smpd_err_printf("unexpected sock operation while waiting for re-connection: op_type = %d\n", event.op_type);
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("accepting reconnection.\n");
	result = sock_accept(listener, session_set, NULL, &session_sock);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_accept failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("accepted sock %d into set %d\n", sock_getid(session_sock), sock_getsetid(session_set));
	smpd_dbg_printf("closing the manager's listener set.\n");
	/* Are we allowed to post a close on the listener?
	result = sock_post_close(listener);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_post_close(listener) failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock failure waiting for listener to close, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	if (event.op_type != SOCK_OP_CLOSE)
	{
	    smpd_err_printf("unexpected sock operation while waiting for listener to close: op_type = %d\n", event.op_type);
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	*/
	smpd_dbg_printf("destroying manager listener set: %d\n", sock_getsetid(set));
	result = sock_destroy_set(set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("Unable to destroy the listener set, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
	    return SMPD_FAIL;
	}
	result = smpd_session(session_set, session_sock);
	if (result != SMPD_SUCCESS)
	    smpd_err_printf("smpd_session() failed.\n");
	
	result = sock_finalize();
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
	}
	smpd_dbg_printf("exiting smpd_parse_command_args (ExitProcess).\n");
	ExitProcess(0);
    }
#endif

    smpd_dbg_printf("exiting smpd_parse_command_args.\n");
    return SMPD_SUCCESS;
}
