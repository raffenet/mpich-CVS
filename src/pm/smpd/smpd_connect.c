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

int smpd_close_connection(sock_set_t set, sock_t sock)
{
    int result;
    sock_event_t event;

    /* close the sock and its set */
    result = sock_post_close(sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error closing socket: %s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }
    do
    {
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("error waiting for socket to close: %s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	/* there may be posted reads, writes, or accepts that may come before the SOCK_OP_CLOSE
	if (event.op_type != SOCK_OP_CLOSE)
	{
	    smpd_err_printf("incorrect sock operation returned: %d\n", event.op_type);
	    return SMPD_FAIL;
	}
	*/
    } while (event.op_type != SOCK_OP_CLOSE);
    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error destroying set: %s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

void smpd_get_password(char *password)
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
	smpd_err_printf("\nunable to turn off the terminal echo\n");
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

void smpd_get_account_and_password(char *account, char *password)
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

    smpd_get_password(password);
}

int smpd_get_credentials_from_parent(sock_set_t set, sock_t sock)
{
    return SMPD_FAIL;
}

int smpd_get_smpd_password_from_parent(sock_set_t set, sock_t sock)
{
    return SMPD_FAIL;
}

int smpd_connect_to_smpd(sock_set_t parent_set, sock_t parent_sock, char *host, char *session_type, sock_set_t *set_ptr, sock_t *sock_ptr)
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
	smpd_err_printf("sock_create_set failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }

    result = sock_post_connect(set, NULL, host, port, &sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }
    if (event.op_type != SOCK_OP_CONNECT)
    {
	smpd_err_printf("sock_wait returned op_type=%d\n", event.op_type);
	return -1;
    }

    /* authenticate the connection */
    result = smpd_authenticate(set, sock, SMPD_CLIENT_AUTHENTICATION);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_authenticate(CLIENT) failed\n");
	return result;
    }

    /* request a session */
    result = smpd_write_string(set, sock, session_type);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_write_string('%s') failed\n", session_type);
	return result;
    }

    if (strcmp(session_type, SMPD_PROCESS_SESSION_STR) == 0)
    {
	/* provide credentials if necessary */
	result = smpd_read_string(set, sock, cred_request, 100);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("smpd_read_string(cred_request) failed\n");
	    return result;
	}
	if (strcmp(cred_request, SMPD_CRED_REQUEST) == 0)
	{
	    if (parent_sock != SOCK_INVALID_SOCK)
	    {
		result = smpd_get_credentials_from_parent(parent_set, parent_sock);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to get the user credentials from the parent.\n");
		    return result;
		}
	    }
	    else
	    {
		smpd_get_account_and_password(account, password);
	    }

	    if (strcmp(account, "invalid account") == 0)
	    {
		smpd_err_printf("attempting to create a session with an smpd that requires credentials without having obtained any credentials.\n");
		return -1;
	    }
	    result = smpd_write_string(set, sock, account);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write_string('%s') failed\n", account);
		return result;
	    }
	    result = smpd_write_string(set, sock, password);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write_string('***') failed\n");
		return result;
	    }
	}

	/* Receive the port re-connect request and reconnect if necessary. */
	/* -1 means that no re-connect is necessary. */
	result = smpd_read_string(set, sock, port_str, 20);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to read the reconnect port string request.\n");
	    return result;
	}
	smpd_dbg_printf("reconnect request to port: %s\n", port_str);
	if (strcmp(port_str, SMPD_NO_RECONNECT_PORT_STR))
	{
	    smpd_dbg_printf("closing the old socket.\n");
	    /* close the old sock */
	    result = sock_post_close(sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_post_close failed, sock error:\n%s\n", get_sock_error_string(result));
		return result;
	    }
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
		return result;
	    }
	    if (event.op_type != SOCK_OP_CLOSE)
	    {
		smpd_err_printf("While closing the old socket, sock_wait returned op_type=%d\n", event.op_type);
		return -1;
	    }

	    smpd_dbg_printf("connecting a new socket.\n");
	    /* reconnect */
	    port = atol(port_str);
	    if (port < 1)
	    {
		smpd_err_printf("Invalid reconnect port read: %d\n", port);
		return -1;
	    }
	    result = sock_post_connect(set, NULL, host, port, &sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
		return result;
	    }
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
		return result;
	    }
	    if (event.op_type != SOCK_OP_CONNECT)
	    {
		smpd_err_printf("sock_wait returned op_type=%d\n", event.op_type);
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
	    smpd_err_printf("smpd_read_string(smpd_cred_request) failed\n");
	    return result;
	}
	if (strcmp(cred_request, SMPD_PWD_REQUEST) == 0)
	{
	    if (parent_sock != SOCK_INVALID_SOCK)
	    {
		result = smpd_get_smpd_password_from_parent(parent_set, parent_sock);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to get the password from the parent.\n");
		    return result;
		}
	    }
	    else
	    {
		fprintf(stderr, "password needed to manage the smpd.\n");
		fprintf(stderr, "password: ");
		fflush(stderr);
		smpd_get_password(password);
	    }

	    result = smpd_write_string(set, sock, password);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write_string('***') failed\n");
		return result;
	    }
	    result = smpd_read_string(set, sock, response, 100);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_read_string(response) failed\n");
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
	smpd_err_printf("Invalid session requested: '%s'\n", session_type);
	return SMPD_FAIL;
    }

    *set_ptr = set;
    *sock_ptr = sock;

    return SMPD_SUCCESS;
}
