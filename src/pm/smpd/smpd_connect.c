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

#ifdef HAVE_SIGACTION
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#endif

smpd_process_t smpd_process = { -1, -1, -1, NULL, NULL, NULL, 0, SOCK_INVALID_SET, "", "", 0, 0, "", "", "" };

#ifdef HAVE_SIGACTION
void child_handler(int code)
{
    int status;
    if (code == SIGCHLD)
	waitpid(-1, &status, WNOHANG);
}
#endif

int smpd_init_process(void)
{
#ifdef HAVE_WINDOWS_H
    HMODULE hModule;
#endif
#ifdef HAVE_SIGACTION
    struct sigaction act;
#endif

    smpd_dbg_printf("entering smpd_init_process.\n");

    /* tree data */
    smpd_process.parent_id = -1;
    smpd_process.id = -1;
    smpd_process.level = -1;
    smpd_process.left_context = NULL;
    smpd_process.right_context = NULL;
    smpd_process.parent_context = NULL;
    smpd_process.set = SOCK_INVALID_SET;

    /* local data */
#ifdef HAVE_WINDOWS_H
    hModule = GetModuleHandle(NULL);
    if (!GetModuleFileName(hModule, smpd_process.pszExe, SMPD_MAX_EXE_LENGTH)) 
	smpd_process.pszExe[0] = '\0';
#else
    smpd_process.pszExe[0] = '\0';
#endif
    strcpy(smpd_process.SMPDPassword, SMPD_DEFAULT_PASSWORD);
    smpd_process.bPasswordProtect = SMPD_FALSE;
    smpd_process.bService = SMPD_FALSE;
    gethostname(smpd_process.host, SMPD_MAX_HOST_LENGTH);
    smpd_process.UserAccount[0] = '\0';
    smpd_process.UserPassword[0] = '\0';
    smpd_process.closing = SMPD_FALSE;

    srand(smpd_getpid());

#ifdef HAVE_SIGACTION
    act.sa_handler = child_handler;
    act.sa_flags = SA_NOCLDSTOP | SA_NOMASK;
    sigaction(SIGCHLD, &act, NULL);
#endif

    smpd_dbg_printf("exiting smpd_init_process.\n");
    return SMPD_SUCCESS;
}

int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, sock_set_t set, sock_t sock, int id)
{
    smpd_dbg_printf("entering smpd_init_context.\n");
    context->type = type;
    context->host[0] = '\0';
    context->id = id;
    context->write_list = NULL;
    smpd_init_command(&context->read_cmd);
    context->next = NULL;
    context->set = set;
    context->sock = sock;
    smpd_dbg_printf("exiting smpd_init_context.\n");
    return SMPD_SUCCESS;
}

int smpd_generate_session_header(char *str, int session_id)
{
    char * str_orig;
    int result;
    int len;

    smpd_dbg_printf("entering smpd_generate_session_header.\n");

    str_orig = str;
    *str = '\0';
    len = SMPD_MAX_SESSION_HEADER_LENGTH;

    /* add header fields */
    result = smpd_add_int_arg(&str, &len, "id", session_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session id failed.\n");
	smpd_dbg_printf("exiting smpd_generate_session_header.\n");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "parent", smpd_process.id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding parent id failed.\n");
	smpd_dbg_printf("exiting smpd_generate_session_header.\n");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "level", smpd_process.level + 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session level failed.\n");
	smpd_dbg_printf("exiting smpd_generate_session_header.\n");
	return SMPD_FAIL;
    }

    /* remove the trailing space */
    str--;
    *str = '\0';

    smpd_dbg_printf("session header: (%s)\n", str_orig);
    smpd_dbg_printf("exiting smpd_generate_session_header.\n");
    return SMPD_SUCCESS;
}

int smpd_close_connection(sock_set_t set, sock_t sock)
{
    int result;
    sock_event_t event;

    smpd_dbg_printf("entering smpd_close_connection.\n");

    /* close the sock and its set */
    smpd_dbg_printf("closing sock\n");
    result = sock_post_close(sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error closing socket: %s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_close_connection.\n");
	return SMPD_FAIL;
    }
    do
    {
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("error waiting for socket to close: %s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_close_connection.\n");
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
    smpd_dbg_printf("sock closed, destroying set\n");
    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error destroying set: %s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_close_connection.\n");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("exiting smpd_close_connection.\n");
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

    smpd_dbg_printf("entering smpd_get_password.\n");

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
    smpd_dbg_printf("exiting smpd_get_password.\n");
}

void smpd_get_account_and_password(char *account, char *password)
{
    size_t len;

    smpd_dbg_printf("entering smpd_get_account_and_password.\n");

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
    smpd_dbg_printf("exiting smpd_get_account_and_password.\n");
}

int smpd_get_credentials_from_parent(sock_set_t set, sock_t sock)
{
    smpd_dbg_printf("entering smpd_get_credentials_from_parent.\n");
    smpd_dbg_printf("exiting smpd_get_credentials_from_parent.\n");
    return SMPD_FAIL;
}

int smpd_get_smpd_password_from_parent(sock_set_t set, sock_t sock)
{
    smpd_dbg_printf("entering smpd_get_smpd_password_from_parent.\n");
    smpd_dbg_printf("exiting smpd_get_smpd_password_from_parent.\n");
    return SMPD_FAIL;
}

int smpd_connect_to_smpd(sock_set_t parent_set, sock_t parent_sock, char *host, char *session_type, int session_id, sock_set_t *set_ptr, sock_t *sock_ptr)
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
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];

    smpd_dbg_printf("entering smpd_connect_to_smpd.\n");

    if (*set_ptr != SOCK_INVALID_SET)
    {
	set = *set_ptr;
    }
    else
    {
	smpd_dbg_printf("creating new set for the smpd connection to %s\n", host);
	result = sock_create_set(&set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	    return result;
	}
    }

    /* This is going to be a problem because sock_wait can return events from other connections */
    /* It would be nice to have a blocking sock_connect call */
    result = sock_post_connect(set, NULL, host, port, &sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return result;
    }
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return result;
    }
    if (event.op_type != SOCK_OP_CONNECT)
    {
	smpd_err_printf("sock_wait returned op_type=%d\n", event.op_type);
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return -1;
    }

    /* authenticate the connection */
    result = smpd_authenticate(set, sock, SMPD_CLIENT_AUTHENTICATION);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_authenticate(CLIENT) failed\n");
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return result;
    }

    /* request a session */
    result = smpd_write_string(set, sock, session_type);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_write_string('%s') failed\n", session_type);
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return result;
    }

    if (strcmp(session_type, SMPD_PROCESS_SESSION_STR) == 0)
    {
	/* provide credentials if necessary */
	result = smpd_read_string(set, sock, cred_request, 100);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("smpd_read_string(cred_request) failed\n");
	    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
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
		    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
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
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return -1;
	    }
	    result = smpd_write_string(set, sock, account);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write_string('%s') failed\n", account);
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    result = smpd_write_string(set, sock, password);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write_string('***') failed\n");
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	}

	/* Receive the port re-connect request and reconnect if necessary. */
	/* -1 means that no re-connect is necessary. */
	result = smpd_read_string(set, sock, port_str, 20);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to read the reconnect port string request.\n");
	    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
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
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    if (event.op_type != SOCK_OP_CLOSE)
	    {
		smpd_err_printf("While closing the old socket, sock_wait returned op_type=%d\n", event.op_type);
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return -1;
	    }

	    smpd_dbg_printf("connecting a new socket.\n");
	    /* reconnect */
	    port = atol(port_str);
	    if (port < 1)
	    {
		smpd_err_printf("Invalid reconnect port read: %d\n", port);
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return -1;
	    }
	    result = sock_post_connect(set, NULL, host, port, &sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    if (event.op_type != SOCK_OP_CONNECT)
	    {
		smpd_err_printf("sock_wait returned op_type=%d\n", event.op_type);
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
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
	    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
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
		    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
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
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    result = smpd_read_string(set, sock, response, 100);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_read_string(response) failed\n");
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return result;
	    }
	    if (strcmp(response, SMPD_AUTHENTICATION_ACCEPTED_STR))
	    {
		printf("management session rejected, incorrect password.\n");
		smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
		return SMPD_FAIL;
	    }
	}
    }
    else
    {
	smpd_err_printf("Invalid session requested: '%s'\n", session_type);
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return SMPD_FAIL;
    }

    /* generate and send the session header */
    result = smpd_generate_session_header(session_header, session_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return -1;
    }
    result = smpd_write_string(set, sock, session_header);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to send the session header.\n");
	smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
	return -1;
    }

    *set_ptr = set;
    *sock_ptr = sock;

    smpd_dbg_printf("exiting smpd_connect_to_smpd.\n");
    return SMPD_SUCCESS;
}
