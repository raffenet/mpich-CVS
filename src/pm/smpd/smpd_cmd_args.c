/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_parse_command_args(int *argcp, char **argvp[])
{
#ifdef HAVE_WINDOWS_H
    char str[20], read_handle_str[20], write_handle_str[20];
    int port;
    sock_t listener;
    sock_set_t set;
    int result;
    HANDLE hWrite, hRead;
    DWORD num_written, num_read;
#endif

    smpd_enter_fn("smpd_parse_command_args");

    if (smpd_get_opt(argcp, argvp, "-d"))
    {
	smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_PREPEND_RANK;
    }

#ifdef HAVE_WINDOWS_H
    if (smpd_get_opt(argcp, argvp, "-mgr"))
    {
	if (!smpd_get_opt_string(argcp, argvp, "-read", read_handle_str, 20))
	{
	    smpd_err_printf("manager started without a read pipe handle.\n");
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (!smpd_get_opt_string(argcp, argvp, "-write", write_handle_str, 20))
	{
	    smpd_err_printf("manager started without a write pipe handle.\n");
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	hRead = smpd_decode_handle(read_handle_str);
	hWrite = smpd_decode_handle(write_handle_str);

	smpd_dbg_printf("manager creating listener and session sets.\n");

	result = sock_create_set(&set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set(listener) failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_process.set = set;
	smpd_dbg_printf("created set for manager listener, %d\n", sock_getsetid(set));
	port = 0;
	result = sock_listen(set, NULL, &port, &listener); 
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_listen failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("smpd manager listening on port %d\n", port);

	result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a context for the smpd listener.\n");
	    smpd_exit_fn("smpd_parse_command_args");
	    return result;
	}
	result = sock_set_user_ptr(listener, smpd_process.listener_context);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_set_user_ptr failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return result;
	}
	smpd_process.listener_context->state = SMPD_MGR_LISTENING;

	memset(str, 0, 20);
	snprintf(str, 20, "%d", port);
	smpd_dbg_printf("manager writing port back to smpd.\n");
	if (!WriteFile(hWrite, str, 20, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	CloseHandle(hWrite);
	if (num_written != 20)
	{
	    smpd_err_printf("wrote only %d bytes of 20\n", num_written);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("manager reading account and password from smpd.\n");
	if (!ReadFile(hRead, smpd_process.UserAccount, 100, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != 100)
	{
	    smpd_err_printf("read only %d bytes of 100\n", num_read);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.UserPassword, 100, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != 100)
	{
	    smpd_err_printf("read only %d bytes of 100\n", num_read);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}

	result = smpd_enter_at_state(set, SMPD_MGR_LISTENING);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("state machine failed.\n");
	}

	result = sock_finalize();
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
	}
	smpd_exit_fn("smpd_parse_command_args (ExitProcess)");
	ExitProcess(0);
    }
#endif

    smpd_exit_fn("smpd_parse_command_args");
    return SMPD_SUCCESS;
}
