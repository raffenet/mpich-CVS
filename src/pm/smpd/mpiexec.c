/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#ifndef HAVE_WINDOWS_H
#include <unistd.h>
#endif

int mp_err_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    va_start(list, str);
    format_str = str;
    n = vfprintf(stderr, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

int mp_parse_command_args(int argc, char *argv[])
{
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
void mp_get_account_and_password(char *account, char *password)
{
    char ch = 0;
    int index = 0;
    HANDLE hStdin;
    DWORD dwMode;

    fprintf(stderr, "mpiexec needs an account to launch processes with:\n");
    do
    {
	fprintf(stderr, "account (domain\\user): ");
	fflush(stderr);
	gets(account);
    } 
    while (strlen(account) == 0);
    
    fprintf(stderr, "password: ");
    fflush(stderr);
    
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    gets(password);
    SetConsoleMode(hStdin, dwMode);
    
    fprintf(stderr, "\n");
}
#endif

int main(int argc, char* argv[])
{
    int result;
    sock_set_t set;
    sock_t sock;
    sock_event_t event;
    char port_str[20];
    int port = SMPD_LISTENER_PORT;
    char host[SMPD_MAX_HOST_LENGTH];
    char account[100], password[100];
    char cred_request[100];

    result = mp_parse_command_args(argc, argv);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to parse the command arguments.\n");
	return result;
    }

    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_create_set failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }

    gethostname(host, SMPD_MAX_HOST_LENGTH);

#ifdef HAVE_WINDOWS_H
    mp_get_account_and_password(account, password);
#else
    strcpy(account, "invalid user");
    strcpy(password, "invalid password");
#endif

    /* connect to the first host - for starters, myself */
    result = sock_post_connect(set, NULL, host, port, &sock);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    if (event.op_type != SOCK_OP_CONNECT)
    {
	mp_err_printf("sock_wait returned op_type=%d\n", event.op_type);
	return -1;
    }

    /* authenticate the connection */
    result = smpd_authenticate(set, sock, SMPD_CLIENT_AUTHENTICATION);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_authenticate(CLIENT) failed\n");
	return result;
    }

    /* request a process session */
    result = smpd_write_string(set, sock, SMPD_PROCESS_SESSION_STR);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_write_string('" SMPD_PROCESS_SESSION_STR "') failed\n");
	return result;
    }

    /* provide credentials if necessary */
    result = smpd_read_string(set, sock, cred_request, 100);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_read_string(cred_request) failed\n");
	return result;
    }
    if (strcmp(cred_request, SMPD_CRED_REQUEST) == 0)
    {
	if (strcmp(account, "invalid account") == 0)
	{
	    mp_err_printf("attempting to create a session with an smpd that requires credentials without having obtained any credentials.\n");
	    return -1;
	}
	result = smpd_write_string(set, sock, account);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("smpd_write_string('%s') failed\n", account);
	    return result;
	}
	result = smpd_write_string(set, sock, password);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("smpd_write_string('***') failed\n");
	    return result;
	}
    }

    /* Receive the port re-connect request and reconnect if necessary. */
    /* -1 means that no re-connect is necessary. */
    result = smpd_read_string(set, sock, port_str, 20);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to read the reconnect port string request.\n");
	return result;
    }
    smpd_dbg_printf("port string request: %s\n", port_str);
    if (strcmp(port_str, "-1"))
    {
	smpd_dbg_printf("closing the old socket.\n");
	/* close the old sock */
	result = sock_post_close(sock);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_post_close failed, sock error:\n%s\n", get_sock_error_string(result));
	    return result;
	}
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    return result;
	}
	if (event.op_type != SOCK_OP_CLOSE)
	{
	    mp_err_printf("While closing the old socket, sock_wait returned op_type=%d\n", event.op_type);
	    return -1;
	}

	smpd_dbg_printf("connecting a new socket.\n");
	/* reconnect */
	port = atol(port_str);
	if (port < 1)
	{
	    mp_err_printf("Invalid reconnect port read: %d\n", port);
	    return -1;
	}
	result = sock_post_connect(set, NULL, host, port, &sock);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
	    return result;
	}
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    return result;
	}
	if (event.op_type != SOCK_OP_CONNECT)
	{
	    mp_err_printf("sock_wait returned op_type=%d\n", event.op_type);
	    return -1;
	}
    }

    smpd_dbg_printf("sending close command\n");
    result = smpd_write_string(set, sock, "close");
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("write_string('close') failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }

    smpd_dbg_printf("closing the sock\n");
    result = sock_post_close(sock);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_post_close failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    if (event.op_type != SOCK_OP_CLOSE)
    {
	mp_err_printf("sock_wait returned op_type=%d\n", event.op_type);
	return -1;
    }
    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_destroy_set failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }

    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }
    return 0;
}

