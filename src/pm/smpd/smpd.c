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
    sock_set_t set;
    sock_t listener;
    int port = SMPD_LISTENER_PORT;

    smpd_enter_fn("main");

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

#ifdef HAVE_WINDOWS_H
    /* prevent the os from bringing up debug message boxes if this process crashes */
    if (smpd_process.bService)
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif

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
    smpd_process.set = set;
    smpd_dbg_printf("created a set for the listener: %d\n", sock_getsetid(set));
    result = sock_listen(set, NULL, &port, &listener); 
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_listen failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }
    smpd_dbg_printf("smpd listening on port %d\n", port);

    result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the smpd listener.\n");
	smpd_exit_fn("main");
	return result;
    }
    result = sock_set_user_ptr(listener, smpd_process.listener_context);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }
    smpd_process.listener_context->state = SMPD_SMPD_LISTENING;

    result = smpd_enter_at_state(set, SMPD_SMPD_LISTENING);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("state machine failed.\n");
    }

    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to destroy the set, error:\n%s\n",
	    get_sock_error_string(result));
    }

    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }
    smpd_exit_fn("main");
    return 0;
}

