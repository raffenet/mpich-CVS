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

int mp_dbg_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    va_start(list, str);
    format_str = str;
    n = vfprintf(stdout, format_str, list);
    va_end(list);

    fflush(stdout);

    return n;
}
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

int handle_command(smpd_context_t *context)
{
    int result;
    smpd_context_t *dest;
    smpd_command_t *cmd;

    mp_dbg_printf("entering handle_command.\n");

    cmd = &context->read_cmd;
    
    mp_dbg_printf("handle_command:\n");
    mp_dbg_printf(" src  = %d\n", cmd->src);
    mp_dbg_printf(" dest = %d\n", cmd->dest);
    mp_dbg_printf(" cmd  = %s\n", cmd->cmd_str);
    mp_dbg_printf(" str = %s\n", cmd->cmd);
    mp_dbg_printf(" len = %d\n", cmd->length);
    if (context == smpd_process.left_context)
	mp_dbg_printf(" context = left\n");
    else if (context == smpd_process.right_context)
	mp_dbg_printf(" context = right\n");
    else if (context == smpd_process.parent_context)
	mp_dbg_printf(" context = parent\n");
    else
	mp_dbg_printf(" context = unknown\n");

    result = smpd_command_destination(cmd->dest, &dest);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("invalid command received, unable to determine the destination.\n");
	mp_dbg_printf("exiting handle_command.\n");
	return SMPD_FAIL;
    }
    if (dest)
    {
	mp_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to forward the command.\n");
	    mp_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	mp_dbg_printf("exiting handle_command.\n");
	return SMPD_SUCCESS;
    }
    if ((strcmp(cmd->cmd_str, "closed") == 0) || (strcmp(cmd->cmd_str, "down") == 0))
    {
	mp_dbg_printf("handling %s command, posting close of the sock.\n", cmd->cmd_str);
	result = sock_post_close(context->sock);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("unable to post a close of the sock after receiving a '%s' command, sock error:\n%s\n",
		cmd->cmd_str, get_sock_error_string(result));
	    mp_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	mp_dbg_printf("exiting handle_command.\n");
	return SMPD_CLOSE;
    }

    mp_err_printf("ignoring unknown command from the session: '%s'\n", cmd->cmd);

    mp_dbg_printf("exiting handle_command.\n");
    return SMPD_SUCCESS;
}

int handle_read(smpd_context_t *context, int num_read, int error, smpd_context_t *session_context)
{
    int result;
    static int read_offset = 0;
    smpd_command_t *cmd_ptr;
    int ret_val = SMPD_SUCCESS;

    mp_dbg_printf("entering handle_read.\n");

    if (error != SOCK_SUCCESS)
    {
	if (context != NULL)
	    mp_err_printf("sock read error on sock connected to '%s':\n%s\n", context->host, get_sock_error_string(error));
	else
	    mp_err_printf("sock read error:\n%s\n", get_sock_error_string(error));
	mp_dbg_printf("exiting handle_read.\n");
	return SMPD_FAIL;
    }
    if (context == NULL)
    {
	mp_err_printf("Error: read on a NULL context of %d bytes\n", num_read);
	mp_dbg_printf("exiting handle_read.\n");
	return SMPD_FAIL;
    }
    if (num_read < 1)
    {
	mp_err_printf("Error: read %d bytes from '%s'\n", num_read, context->host);
	mp_dbg_printf("exiting handle_read.\n");
	return SMPD_FAIL;
    }
    if (context->host[0] == '\0')
    {
	/* handle data read from stdin */
	if (context->read_cmd.cmd[read_offset] == '\n')
	{
	    context->read_cmd.cmd[read_offset] = '\0'; /* remove the \n character */
	    mp_dbg_printf("command read from stdin, forwarding to smpd\n");
	    result = smpd_parse_command(&context->read_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("invalid command read from stdin, ignoring: \"%s\"\n", context->read_cmd.cmd);
		mp_dbg_printf("exiting handle_read.\n");
		return SMPD_FAIL;
	    }
	    result = smpd_create_command_copy(&context->read_cmd, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to create a copy of the command read from stdin: \"%s\"\n", context->read_cmd.cmd);
		mp_dbg_printf("exiting handle_read.\n");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(session_context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to post a write of the command read from stdin: \"%s\"\n", cmd_ptr->cmd);
		smpd_free_command(cmd_ptr);
		mp_dbg_printf("exiting handle_read.\n");
		return SMPD_FAIL;
	    }
	    mp_dbg_printf("posted write of command: \"%s\"\n", cmd_ptr->cmd);
	    read_offset = 0;
	}
	else
	{
	    read_offset++;
	}
	result = sock_post_read(context->sock, &context->read_cmd.cmd[read_offset], 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("unable to post a read on the stdin sock, error:\n%s\n", get_sock_error_string(result));
	    mp_dbg_printf("exiting handle_read.\n");
	    return SMPD_FAIL;
	}
    }
    else
    {
	switch (context->read_cmd.state)
	{
	case SMPD_CMD_INVALID:
	    mp_err_printf("data read on a command in the invalid state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READING_HDR:
	    context->read_cmd.length = atoi(context->read_cmd.cmd_hdr_str);
	    if (context->read_cmd.length < 1)
	    {
		mp_err_printf("unable to read the command, invalid command length: %d\n", context->read_cmd.length);
		ret_val = SMPD_FAIL;
		break;
	    }
	    mp_dbg_printf("read command header, posting read of data: %d bytes\n", context->read_cmd.length);
	    context->read_cmd.state = SMPD_CMD_READING_CMD;
	    ret_val = sock_post_read(context->sock, context->read_cmd.cmd, context->read_cmd.length, NULL);
	    if (ret_val == SOCK_SUCCESS)
		ret_val = SMPD_SUCCESS;
	    else
	    {
		mp_err_printf("unable to post a read for the command string, sock error:\n%s\n", get_sock_error_string(ret_val));
		ret_val = SMPD_FAIL;
	    }
	    break;
	case SMPD_CMD_READING_CMD:
	    ret_val = smpd_parse_command(&context->read_cmd);
	    if (ret_val != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to parse the read command: \"%s\"\n", context->read_cmd.cmd);
		break;
	    }
	    mp_dbg_printf("read command: \"%s\"\n", context->read_cmd.cmd);
	    ret_val = handle_command(context);
	    if (ret_val == SMPD_SUCCESS)
	    {
		ret_val = smpd_post_read_command(context);
	    }
	    else
	    {
		if (ret_val != SMPD_CLOSE)
		{
		    mp_err_printf("unable to handle the command: \"%s\"\n", context->read_cmd.cmd);
		}
	    }
	    break;
	case SMPD_CMD_WRITING_CMD:
	    mp_err_printf("data read on a command in the writing_cmd state.\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READY:
	    mp_err_printf("data read on a command in the ready state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_HANDLED:
	    mp_err_printf("data read on a command in the handled state\n");
	    ret_val = SMPD_FAIL;
	    break;
	default:
	    mp_err_printf("data read on a command in an invalid state: %d\n", context->read_cmd.state);
	    ret_val = SMPD_FAIL;
	    break;
	}
	mp_dbg_printf("exiting handle_read.\n");
	return ret_val;
    }
    mp_dbg_printf("exiting handle_read.\n");
    return SMPD_SUCCESS;
}

int handle_written(smpd_context_t *context, int num_written, int error)
{
    smpd_command_t *cmd;
    int ret_val = SMPD_SUCCESS;

    mp_dbg_printf("entering handle_written.\n");

    if (error != SOCK_SUCCESS)
    {
	if (context != NULL)
	    mp_err_printf("sock write error on sock connected to '%s':\n%s\n", context->host, get_sock_error_string(error));
	else
	    mp_err_printf("sock write error:\n%s\n", get_sock_error_string(error));
	mp_dbg_printf("exiting handle_written.\n");
	return SMPD_FAIL;
    }
    if (context == NULL)
    {
	mp_err_printf("Error: write on a NULL context of %d bytes\n", num_written);
	mp_dbg_printf("exiting handle_written.\n");
	return SMPD_FAIL;
    }
    if (context->host[0] == '\0')
    {
	mp_err_printf("Error: unexpected write finished on stdin context.\n");
    }
    else
    {
	if (context->write_list == NULL)
	{
	    mp_err_printf("data written on a context with no write command posted.\n");
	    mp_dbg_printf("exiting handle_written.\n");
	    return SMPD_FAIL;
	}
	switch (context->write_list->state)
	{
	case SMPD_CMD_INVALID:
	    mp_err_printf("data written on a command in the invalid state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READING_HDR:
	    mp_err_printf("data written on a command in the reading_hdr state.\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READING_CMD:
	    mp_err_printf("data written on a command in the reading_cmd state.\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_WRITING_CMD:
	    cmd = context->write_list;
	    context->write_list = context->write_list->next;
	    ret_val = smpd_free_command(cmd);
	    if (ret_val != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to free the written command.\n");
		break;
	    }
	    cmd = context->write_list;
	    if (cmd)
	    {
		cmd->iov[0].SOCK_IOV_BUF = cmd->cmd_hdr_str;
		cmd->iov[0].SOCK_IOV_LEN = SMPD_CMD_HDR_LENGTH;
		cmd->iov[1].SOCK_IOV_BUF = cmd->cmd;
		cmd->iov[1].SOCK_IOV_LEN = cmd->length;
		ret_val = sock_post_writev(context->sock, cmd->iov, 2, NULL);
		if (ret_val != SOCK_SUCCESS)
		{
		    mp_err_printf("unable to post a write for the next command, sock error:\n%s\n", get_sock_error_string(ret_val));
		    ret_val = SMPD_FAIL;
		}
	    }
	    break;
	case SMPD_CMD_READY:
	    mp_err_printf("data written on a command in the ready state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_HANDLED:
	    mp_err_printf("data written on a command in the handled state\n");
	    ret_val = SMPD_FAIL;
	    break;
	default:
	    mp_err_printf("data written on a command in an invalid state: %d\n", context->write_list->state);
	    ret_val = SMPD_FAIL;
	    break;
	}
    }
    mp_dbg_printf("wrote %d bytes\n", num_written);
    mp_dbg_printf("exiting handle_written.\n");
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
HANDLE g_hCloseStdinThreadEvent = NULL;
HANDLE g_hStdinThread = NULL;
void StdinThread(SOCKET hWrite)
{
    DWORD len;
    char str[SMPD_MAX_CMD_LENGTH];
    HANDLE h[2];
    int result;

    h[0] = GetStdHandle(STD_INPUT_HANDLE);
    if (h[0] == NULL)
    {
	mp_err_printf("Unable to get the stdin handle.\n");
	return;
    }
    h[1] = g_hCloseStdinThreadEvent;
    while (1)
    {
	/*mp_dbg_printf("waiting for input from stdin\n");*/
	result = WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (result == WAIT_OBJECT_0)
	{
	    fgets(str, SMPD_MAX_CMD_LENGTH, stdin);
	    len = (DWORD)strlen(str);
	    mp_dbg_printf("forwarding stdin: '%s'\n", str);
	    if (send(hWrite, str, len, 0) == SOCKET_ERROR)
	    {
		mp_err_printf("unable to forward stdin, WriteFile failed, error %d\n", GetLastError());
		return;
	    }
	    /*
	    if (strncmp(str, "close", 5) == 0)
	    {
		shutdown(hWrite, SD_BOTH);
		closesocket(hWrite);
		mp_dbg_printf("closing stdin reader thread.\n");
		return;
	    }
	    */
	}
	else if (result == WAIT_OBJECT_0 + 1)
	{
	    shutdown(hWrite, SD_BOTH);
	    closesocket(hWrite);
	    mp_dbg_printf("g_hCloseStdinThreadEvent signalled, closing stdin reader thread.\n");
	    return;
	}
	else
	{
	    mp_err_printf("stdin wait failed, error %d\n", GetLastError());
	    return;
	}
    }
}

int MakeSockLoop(SOCKET *pRead, SOCKET *pWrite)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    LINGER linger;
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
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

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

int mp_console(char *host)
{
    int result;
    sock_set_t set;
    sock_t sock, insock;
    sock_event_t event;
    SOCK_NATIVE_FD stdin_fd;
    smpd_context_t *context;
    smpd_context_t *session_context;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif

    mp_dbg_printf("entering mp_console.\n");

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;

    /* create a session with the host */
    set = SOCK_INVALID_SET;
    /*result = smpd_connect_to_smpd(SOCK_INVALID_SET, SOCK_INVALID_SOCK, host, SMPD_SMPD_SESSION_STR, 1, &set, &sock);*/
    result = smpd_connect_to_smpd(SOCK_INVALID_SET, SOCK_INVALID_SOCK, host, SMPD_PROCESS_SESSION_STR, 1, &set, &sock);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to connect to smpd on %s\n", host);
	mp_dbg_printf("exiting mp_console.\n");
	return result;
    }

    /* create a context for the session */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }
    smpd_init_context(context, SMPD_CONTEXT_CHILD, set, sock, 1);
    strcpy(context->host, host);
    sock_set_user_ptr(sock, context);
    session_context = context;
    smpd_process.left_context = context;

    /* post a read for a possible incoming command */
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to post a read for an incoming command from the smpd on '%s', error:\n%s\n",
	    context->host, get_sock_error_string(result));
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }

    /* create a context for reading from stdin */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }

    /* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
    result = MakeSockLoop(&stdin_fd, &hWrite);
    if (result)
    {
	mp_err_printf("Unable to make a local socket loop to forward stdin.\n");
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }
#else
    stdin_fd = fileno(stdin);
#endif

    /* convert the native handle to a sock */
    result = sock_native_to_sock(set, stdin_fd, context, &insock);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("unable to create a sock from stdin, sock error:\n%s\n", get_sock_error_string(result));
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }
    smpd_init_context(context, SMPD_CONTEXT_STDIN, set, insock, -1);

#ifdef HAVE_WINDOWS_H
    /* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
       stdin to a sock */
    g_hCloseStdinThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_hCloseStdinThreadEvent == NULL)
    {
	mp_err_printf("Unable to create the stdin thread close event, error %d\n", GetLastError());
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }
    g_hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StdinThread, (void*)hWrite, 0, &dwThreadID);
    if (g_hStdinThread == NULL)
    {
	mp_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }
#endif

    /* post a read for a user command from stdin */
    result = sock_post_read(insock, context->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
	    get_sock_error_string(result));
	mp_dbg_printf("exiting mp_console.\n");
	return SMPD_FAIL;
    }

    /* wait for and handle commands */
    result = SMPD_SUCCESS;
    while (result == SMPD_SUCCESS)
    {
	mp_dbg_printf("sock_waiting for next event.\n");
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    mp_dbg_printf("exiting mp_console.\n");
	    return SMPD_FAIL;
	}

	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    result = handle_read(event.user_ptr, event.num_bytes, event.error, session_context);
	    if (result == SMPD_CLOSE)
	    {
		mp_dbg_printf("handle_read returned SMPD_CLOSE\n");
		result = SMPD_SUCCESS;
		break;
	    }
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("handle_read() failed.\n");
	    }
	    break;
	case SOCK_OP_WRITE:
	    result = handle_written(event.user_ptr, event.num_bytes, event.error);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("handle_written() failed.\n");
	    }
	    break;
	case SOCK_OP_ACCEPT:
	    mp_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CONNECT:
	    mp_err_printf("unexpected connect event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CLOSE:
	    if (event.user_ptr == smpd_process.left_context)
	    {
		mp_dbg_printf("child context closed.\n");
		free(smpd_process.left_context);
		smpd_dbg_printf("closing the session.\n");
		result = sock_destroy_set(set);
		if (result != SOCK_SUCCESS)
		{
		    mp_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		    mp_dbg_printf("exiting mp_console.\n");
		    return SMPD_FAIL;
		}
		mp_dbg_printf("mp_console returning SMPD_SUCCESS\n");
		mp_dbg_printf("exiting mp_console.\n");
		return SMPD_SUCCESS;
	    }
	    else
	    {
		mp_err_printf("unexpected close event returned by sock_wait.\n");
	    }
	    break;
	default:
	    mp_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
	    break;
	}
    }

    mp_dbg_printf("closing smpd console session connection.\n");
    result = smpd_close_connection(set, sock);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to close the connection to smpd\n");
	mp_dbg_printf("exiting mp_console.\n");
	return result;
    }
    mp_dbg_printf("mp_console returning SMPD_SUCCESS\n");
    mp_dbg_printf("exiting mp_console.\n");
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
