/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
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

int mp_close_connection(sock_set_t set, sock_t sock)
{
    int result;
    sock_event_t event;

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

    return 0;
}

int mp_parse_command_args(int argc, char *argv[])
{
    return SMPD_SUCCESS;
}

void mp_get_password(char *password)
{
    char ch = 0;
    int index = 0;
#ifdef HAVE_WINDOWS_H
    HANDLE hStdin;
    DWORD dwMode;
#else
    struct termios terminal_settings, original_settings;
#endif
    size_t len;

#ifdef HAVE_WINDOWS_H

    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    *password = '\0';
    fgets(password, 100, stdin);
    SetConsoleMode(hStdin, dwMode);

    fprintf(stderr, "\n");

#else

    /* save the current terminal settings */
    tcgetattr(STDIN_FILENO, &terminal_settings);
    original_settings = terminal_settings;

    /* turn off echo */
    terminal_settings.c_lflag &= ~ECHO;
    terminal_settings.c_lflag |= ECHONL;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal_settings);

    /* check that echo is off */
    tcgetattr(STDIN_FILENO, &terminal_settings);
    if (terminal_settings.c_lflag & ECHO)
    {
	mp_err_printf("\nunable to turn off the terminal echo\n");
	tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
    }

    fgets(password, 100, stdin);

    /* restore the original settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);

#endif

    while ((len = strlen(password)) > 0)
    {
	if (password[len-1] == '\r' || password[len-1] == '\n')
	    password[len-1] = '\0';
	else
	    break;
    }
}

void mp_get_account_and_password(char *account, char *password)
{
    size_t len;

    fprintf(stderr, "credentials needed to launch processes:\n");
    do
    {
	fprintf(stderr, "account (domain\\user): ");
	fflush(stderr);
	*account = '\0';
	fgets(account, 100, stdin);
	while (strlen(account))
	{
	    len = strlen(account);
	    if (account[len-1] == '\r' || account[len-1] == '\n')
		account[len-1] = '\0';
	    else
		break;
	}
    } 
    while (strlen(account) == 0);
    
    fprintf(stderr, "password: ");
    fflush(stderr);

    mp_get_password(password);
}

int mp_connect_to_smpd(char *host, char *session_type, sock_set_t *set_ptr, sock_t *sock_ptr)
{
    int result;
    sock_set_t set;
    sock_t sock;
    sock_event_t event;
    char port_str[20];
    int port = SMPD_LISTENER_PORT;
    char account[100] = "invalid account";
    char password[100] = "invalid password";
    char cred_request[100];
    char response[100];

    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_create_set failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
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

    /* authenticate the connection */
    result = smpd_authenticate(set, sock, SMPD_CLIENT_AUTHENTICATION);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_authenticate(CLIENT) failed\n");
	return result;
    }

    /* request a session */
    result = smpd_write_string(set, sock, session_type);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_write_string('%s') failed\n", session_type);
	return result;
    }

    if (strcmp(session_type, SMPD_PROCESS_SESSION_STR) == 0)
    {
	/* provide credentials if necessary */
	result = smpd_read_string(set, sock, cred_request, 100);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("smpd_read_string(cred_request) failed\n");
	    return result;
	}
	if (strcmp(cred_request, SMPD_CRED_REQUEST) == 0)
	{
	    mp_get_account_and_password(account, password);

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
    }
    else if (strcmp(session_type, SMPD_SMPD_SESSION_STR) == 0)
    {
	/* provide credentials if necessary */
	result = smpd_read_string(set, sock, cred_request, 100);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("smpd_read_string(smpd_cred_request) failed\n");
	    return result;
	}
	if (strcmp(cred_request, SMPD_PWD_REQUEST) == 0)
	{
	    fprintf(stderr, "password needed to manage the smpd.\n");
	    fprintf(stderr, "password: ");
	    fflush(stderr);
	    mp_get_password(password);

	    result = smpd_write_string(set, sock, password);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("smpd_write_string('***') failed\n");
		return result;
	    }
	    result = smpd_read_string(set, sock, response, 100);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("smpd_read_string(response) failed\n");
		return result;
	    }
	    if (strcmp(response, SMPD_AUTHENTICATION_ACCEPTED_STR))
	    {
		printf("management session rejected, incorrect password.\n");
		return SMPD_FAIL;
	    }
	}
    }
    else
    {
	mp_err_printf("Invalid session requested: '%s'\n", session_type);
	return SMPD_FAIL;
    }

    *set_ptr = set;
    *sock_ptr = sock;

    return 0;
}

