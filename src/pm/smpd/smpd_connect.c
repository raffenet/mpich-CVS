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
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#endif

smpd_global_t smpd_process = 
    { -1, -1, -1, 
      NULL, NULL, NULL, NULL,
      NULL,
      SMPD_FALSE, SMPD_FALSE,
      SOCK_INVALID_SET,
      "", "",
      SMPD_FALSE, SMPD_FALSE,
      "", "", "",
      0,
      SMPD_DBG_STATE_ERROUT, NULL };

#ifdef HAVE_SIGACTION
void smpd_child_handler(int code)
{
    int status;
    int pid;

    if (code == SIGCHLD)
    {
	/*pid = waitpid(-1, &status, WNOHANG);*/
	pid = waitpid(-1, &status, 0);
	if (pid < 0)
	{
	    fprintf(stderr, "waitpid failed, error %d\n", errno);
	}
	else
	{
	    printf("process %d exited with code: %d\n", pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
	    fflush(stdout);
	}
    }
}
#endif

#ifdef HAVE_WINDOWS_H
int smpd_make_socket_loop(SOCKET *pRead, SOCKET *pWrite)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    /*LINGER linger;*/
    BOOL b;
    SOCKADDR_IN sockAddr;
    int error;

    /* Create a listener */

    /* create the socket */
    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return WSAGetLastError();
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons((unsigned short)ADDR_ANY);
    
    if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	mp_err_printf("bind failed: error %d\n", error);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }
    
    /* listen */
    listen(sock, 2);

    /* get the host and port where we're listening */
    len = sizeof(sockAddr);
    getsockname(sock, (struct sockaddr*)&sockAddr, &len);
    port = ntohs(sockAddr.sin_port);
    gethostname(host, 100);

    /* Connect to myself */

    /* create the socket */
    *pWrite = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (*pWrite == INVALID_SOCKET)
    {
	error = WSAGetLastError();
	mp_err_printf("WSASocket failed, error %d\n", error);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect to myself */
    if (connect(*pWrite, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	closesocket(*pWrite);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* Accept the connection from myself */
    len = sizeof(sockAddr);
    *pRead = accept(sock, (SOCKADDR*)&sockAddr, &len);

    closesocket(sock);
    return 0;
}

int smpd_make_socket_loop_choose(SOCKET *pRead, int read_overlapped, SOCKET *pWrite, int write_overlapped)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    /*LINGER linger;*/
    BOOL b;
    SOCKADDR_IN sockAddr;
    int error;
    DWORD flag;

    /* Create a listener */

    /* create the socket */
    flag = read_overlapped ? WSA_FLAG_OVERLAPPED : 0;
    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, flag);
    if (sock == INVALID_SOCKET)
    {
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return WSAGetLastError();
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons((unsigned short)ADDR_ANY);

    if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	mp_err_printf("bind failed: error %d\n", error);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }
    
    /* listen */
    listen(sock, 2);

    /* get the host and port where we're listening */
    len = sizeof(sockAddr);
    getsockname(sock, (struct sockaddr*)&sockAddr, &len);
    port = ntohs(sockAddr.sin_port);
    gethostname(host, 100);

    /* Connect to myself */

    /* create the socket */
    flag = write_overlapped ? WSA_FLAG_OVERLAPPED : 0;
    *pWrite = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, flag);
    if (*pWrite == INVALID_SOCKET)
    {
	error = WSAGetLastError();
	mp_err_printf("WSASocket failed, error %d\n", error);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect to myself */
    if (connect(*pWrite, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	closesocket(*pWrite);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* Accept the connection from myself */
    len = sizeof(sockAddr);
    *pRead = accept(sock, (SOCKADDR*)&sockAddr, &len);

    closesocket(sock);
    return 0;
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

    smpd_enter_fn("smpd_init_process");

    /* initialize the debugging output print engine */
    smpd_init_printf();

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
    smpd_process.root_smpd = SMPD_FALSE;

    srand(smpd_getpid());

#ifdef HAVE_SIGACTION
    act.sa_handler = smpd_child_handler;
    act.sa_flags = SA_NOCLDSTOP | SA_NOMASK;
    sigaction(SIGCHLD, &act, NULL);
#endif

    smpd_exit_fn("smpd_init_process");
    return SMPD_SUCCESS;
}

int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, sock_set_t set, sock_t sock, int id)
{
    smpd_enter_fn("smpd_init_context");
    context->type = type;
    context->host[0] = '\0';
    context->id = id;
    context->rank = 0;
    context->write_list = NULL;
    context->wait_list = NULL;
    smpd_init_command(&context->read_cmd);
    context->next = NULL;
    context->set = set;
    context->sock = sock;
    smpd_exit_fn("smpd_init_context");
    return SMPD_SUCCESS;
}

int smpd_generate_session_header(char *str, int session_id)
{
    char * str_orig;
    int result;
    int len;

    smpd_enter_fn("smpd_generate_session_header");

    str_orig = str;
    *str = '\0';
    len = SMPD_MAX_SESSION_HEADER_LENGTH;

    /* add header fields */
    result = smpd_add_int_arg(&str, &len, "id", session_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session id failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "parent", smpd_process.id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding parent id failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "level", smpd_process.level + 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session level failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }

    /* remove the trailing space */
    str--;
    *str = '\0';

    smpd_dbg_printf("session header: (%s)\n", str_orig);
    smpd_exit_fn("smpd_generate_session_header");
    return SMPD_SUCCESS;
}

int smpd_close_connection(sock_set_t set, sock_t sock)
{
    int result;
    sock_event_t event;

    smpd_enter_fn("smpd_close_connection");

    /* close the sock and its set */
    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(sock));
    result = sock_post_close(sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error closing sock: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_close_connection");
	return SMPD_FAIL;
    }
    do
    {
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("error waiting for socket to close: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_close_connection");
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
    smpd_dbg_printf("sock closed, destroying set %d\n", sock_getsetid(set));
    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error destroying set: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_close_connection");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_close_connection");
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

    smpd_enter_fn("smpd_get_password");

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
    smpd_exit_fn("smpd_get_password");
}

void smpd_get_account_and_password(char *account, char *password)
{
    size_t len;

    smpd_enter_fn("smpd_get_account_and_password");

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
    smpd_exit_fn("smpd_get_account_and_password");
}

int smpd_get_credentials_from_parent(sock_set_t set, sock_t sock)
{
    smpd_enter_fn("smpd_get_credentials_from_parent");
    smpd_exit_fn("smpd_get_credentials_from_parent");
    return SMPD_FAIL;
}

int smpd_get_smpd_password_from_parent(sock_set_t set, sock_t sock)
{
    smpd_enter_fn("smpd_get_smpd_password_from_parent");
    smpd_exit_fn("smpd_get_smpd_password_from_parent");
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
    char account[SMPD_MAX_ACCOUNT_LENGTH] = "invalid account";
    char password[SMPD_MAX_PASSWORD_LENGTH] = "invalid password";
    char cred_request[SMPD_MAX_CRED_REQUEST_LENGTH];
    char response[SMPD_AUTHENTICATION_REPLY_LENGTH];
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];
    char session_type_block[SMPD_SESSION_REQUEST_LEN];

    smpd_enter_fn("smpd_connect_to_smpd");

    strcpy(session_type_block, session_type);

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
	    smpd_exit_fn("smpd_connect_to_smpd");
	    return result;
	}
    }

    /* This is going to be a problem because sock_wait can return events from other connections */
    /* It would be nice to have a blocking sock_connect call */
    result = sock_post_connect(set, NULL, host, port, &sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_connect_to_smpd");
	return result;
    }
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_connect_to_smpd");
	return result;
    }
    if (event.op_type != SOCK_OP_CONNECT)
    {
	smpd_err_printf("sock_wait returned op_type=%d\n", event.op_type);
	smpd_exit_fn("smpd_connect_to_smpd");
	return -1;
    }

    /* authenticate the connection */
    result = smpd_authenticate(set, sock, SMPD_CLIENT_AUTHENTICATION);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_authenticate(CLIENT) failed\n");
	smpd_exit_fn("smpd_connect_to_smpd");
	return result;
    }

    /* request a session */
    result = smpd_write(sock, session_type_block, SMPD_SESSION_REQUEST_LEN);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_write('%s') failed\n", session_type);
	smpd_exit_fn("smpd_connect_to_smpd");
	return result;
    }

    if (strcmp(session_type, SMPD_PROCESS_SESSION_STR) == 0)
    {
	/* provide credentials if necessary */
	result = smpd_read(sock, cred_request, SMPD_MAX_CRED_REQUEST_LENGTH);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("smpd_read(cred_request) failed\n");
	    smpd_exit_fn("smpd_connect_to_smpd");
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
		    smpd_exit_fn("smpd_connect_to_smpd");
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
		smpd_exit_fn("smpd_connect_to_smpd");
		return -1;
	    }
	    result = smpd_write(sock, account, SMPD_MAX_ACCOUNT_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write('%s') failed\n", account);
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    result = smpd_write(sock, password, SMPD_MAX_PASSWORD_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write('***') failed\n");
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	}

	/* Receive the port re-connect request and reconnect if necessary. */
	/* -1 means that no re-connect is necessary. */
	result = smpd_read(sock, port_str, SMPD_MAX_PORT_STR_LENGTH);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to read the reconnect port string request.\n");
	    smpd_exit_fn("smpd_connect_to_smpd");
	    return result;
	}
	smpd_dbg_printf("reconnect request to port: %s\n", port_str);
	if (strcmp(port_str, SMPD_NO_RECONNECT_PORT_STR))
	{
	    smpd_dbg_printf("closing the old socket.\n");
	    /* close the old sock */
	    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(sock));
	    result = sock_post_close(sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_post_close failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    if (event.op_type != SOCK_OP_CLOSE)
	    {
		smpd_err_printf("While closing the old socket, sock_wait returned op_type=%d\n", event.op_type);
		smpd_exit_fn("smpd_connect_to_smpd");
		return -1;
	    }

	    smpd_dbg_printf("connecting a new socket.\n");
	    /* reconnect */
	    port = atol(port_str);
	    if (port < 1)
	    {
		smpd_err_printf("Invalid reconnect port read: %d\n", port);
		smpd_exit_fn("smpd_connect_to_smpd");
		return -1;
	    }
	    result = sock_post_connect(set, NULL, host, port, &sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_post_connect failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    if (event.op_type != SOCK_OP_CONNECT)
	    {
		smpd_err_printf("sock_wait returned op_type=%d\n", event.op_type);
		smpd_exit_fn("smpd_connect_to_smpd");
		return -1;
	    }
	}
    }
    else if (strcmp(session_type, SMPD_SMPD_SESSION_STR) == 0)
    {
	/* provide credentials if necessary */
	result = smpd_read(sock, cred_request, SMPD_MAX_CRED_REQUEST_LENGTH);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("smpd_read(smpd_cred_request) failed\n");
	    smpd_exit_fn("smpd_connect_to_smpd");
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
		    smpd_exit_fn("smpd_connect_to_smpd");
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

	    result = smpd_write(sock, password, SMPD_MAX_PASSWORD_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_write('***') failed\n");
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    result = smpd_read(sock, response, SMPD_AUTHENTICATION_REPLY_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("smpd_read(response) failed\n");
		smpd_exit_fn("smpd_connect_to_smpd");
		return result;
	    }
	    if (strcmp(response, SMPD_AUTHENTICATION_ACCEPTED_STR))
	    {
		printf("management session rejected, incorrect password.\n");
		smpd_exit_fn("smpd_connect_to_smpd");
		return SMPD_FAIL;
	    }
	}
    }
    else
    {
	smpd_err_printf("Invalid session requested: '%s'\n", session_type);
	smpd_exit_fn("smpd_connect_to_smpd");
	return SMPD_FAIL;
    }

    /* generate and send the session header */
    result = smpd_generate_session_header(session_header, session_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn("smpd_connect_to_smpd");
	return -1;
    }
    result = smpd_write(sock, session_header, SMPD_MAX_SESSION_HEADER_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to send the session header.\n");
	smpd_exit_fn("smpd_connect_to_smpd");
	return -1;
    }

    *set_ptr = set;
    *sock_ptr = sock;

    smpd_exit_fn("smpd_connect_to_smpd");
    return SMPD_SUCCESS;
}
