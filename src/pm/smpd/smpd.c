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

/* global variables */
char g_pszSMPDExe[1024];
int  g_bService = SMPD_FALSE;
int  g_bPasswordProtect = SMPD_FALSE;
char g_SMPDPassword[100] = SMPD_DEFAULT_PASSWORD;
char g_UserAccount[100], g_UserPassword[100];

int main(int argc, char* argv[])
{
    int result;
    sock_set_t set, new_set;
    sock_t listener, new_sock;
    sock_event_t event;
    int port = SMPD_LISTENER_PORT;
#ifdef HAVE_WINDOWS_H
    HMODULE hModule;
#endif

    result = smpd_parse_command_args(argc, argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the command arguments.\n");
	return result;
    }

#ifdef HAVE_WINDOWS_H
    hModule = GetModuleHandle(NULL);
    if (!GetModuleFileName(hModule, g_pszSMPDExe, 1024)) 
	strcpy(g_pszSMPDExe, "smpd.exe");
#else
    strcpy(g_pszSMPDExe, "smpd");
#endif
    smpd_set_smpd_data("path", g_pszSMPDExe);

    srand(smpd_getpid());

    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_create_set failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    result = sock_listen(set, NULL, &port, &listener); 
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_listen failed, sock error:\n%s\n", get_sock_error_string(result));
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
	    smpd_dbg_printf("accepting new socket\n");
	    result = sock_create_set(&new_set);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("error creating a new set for the newly accepted socket:\n%s\n", get_sock_error_string(result));
		break;
	    }
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
    return 0;
}

