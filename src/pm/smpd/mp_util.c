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

int handle_read(smpd_context *context, int num_read, int error, smpd_context *session_context)
{
    int result;
    if (error != SOCK_SUCCESS)
    {
	if (context != NULL)
	    mp_err_printf("sock read error on sock connected to '%s':\n%s\n", context->host, get_sock_error_string(error));
	else
	    mp_err_printf("sock read error:\n%s\n", get_sock_error_string(error));
	return SMPD_FAIL;
    }
    if (context == NULL)
    {
	mp_err_printf("Error: read on a NULL context of %d bytes\n", num_read);
	return SMPD_FAIL;
    }
    if (num_read < 1)
    {
	mp_err_printf("Error: read %d bytes from '%s'\n", num_read, context->host);
	return SMPD_FAIL;
    }
    if (context->host[0] == '\0')
    {
	/* handle data read from stdin */
	/*mp_dbg_printf("[%c]", context->input_str[context->read_offset]);*/
	if (context->input_str[context->read_offset] == '\n')
	{
	    context->input_str[context->read_offset] = '\0';
	    mp_dbg_printf("finished reading command from stdin, forwarding to smpd: %s\n", context->input_str);
	    strcpy(session_context->output_str, context->input_str);
	    result = smpd_package_command(session_context);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to package the command.\n");
		return SMPD_FAIL;
	    }
	    result = smpd_write_command(session_context);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to write the command.\n");
		return SMPD_FAIL;
	    }
	    /*result = smpd_write_string(session_context->set, session_context->sock, session_context->output_str);*/
	    context->read_offset = 0;
	}
	else
	{
	    context->read_offset++;
	}
	result = sock_post_read(context->sock, &context->input_str[context->read_offset], 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("unable to post a read on the stdin sock, error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
    }
    else
    {
	/* hande command read from the session host */
	/*
	if (context->input_str[0] == '\0')
	{
	    mp_err_printf("Error: read an empty command from '%s'\n", context->host);
	    return SMPD_FAIL;
	}
	result = smpd_read_string(context->set, context->sock, &context->input_str[1], SMPD_MAX_CMD_LENGTH-1);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("Error: unable to read a command string from '%s'\n", context->host);
	    return result;
	}
	*/
	smpd_read_command(context);
	mp_dbg_printf("read command from '%s': '%s'\n", context->host, context->input_str);
	if (strncmp(context->input_str, "close", 5) == 0)
	{
	    return SMPD_CLOSE;
	}
	else
	{
	    mp_err_printf("ignoring unknown command from the session: '%s'\n", context->input_str);
	}
	smpd_post_read_command(context);
    }
    return SMPD_SUCCESS;
}

int handle_written(smpd_context *context, int num_written, int error)
{
    if (error != SOCK_SUCCESS)
    {
	if (context != NULL)
	    mp_err_printf("sock write error on sock connected to '%s':\n%s\n", context->host, get_sock_error_string(error));
	else
	    mp_err_printf("sock write error:\n%s\n", get_sock_error_string(error));
	return SMPD_FAIL;
    }
    if (context == NULL)
    {
	mp_err_printf("Error: write on a NULL context of %d bytes\n", num_written);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
void StdinThread(HANDLE hWrite)
{
    size_t len, i;
    char str[SMPD_MAX_CMD_LENGTH], *str_iter;
    DWORD num_written;

    while (1)
    {
	mp_dbg_printf("waiting for input from stdin\n");
	fgets(str, SMPD_MAX_CMD_LENGTH, stdin);
	str_iter = str;
	len = strlen(str);
	mp_dbg_printf("forwarding stdin: '%s'\n", str);
	for (i=0; i<len; i++)
	{
	    if (!WriteFile(hWrite, str_iter, 1, &num_written, NULL))
	    {
		mp_err_printf("unable to forward stdin, WriteFile failed, error %d\n", GetLastError());
		return;
	    }
	    str_iter++;
	}
	FlushFileBuffers(hWrite);
	if (strncmp(str, "close", 5) == 0)
	{
	    CloseHandle(hWrite);
	    mp_dbg_printf("closing stdin reader thread.\n");
	    return;
	}
    }
}

void gen_unique_name(char *str, int len)
{
    UUID guid;
    UuidCreate(&guid);
    snprintf(str, len, "\\\\.\\pipe\\%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}
#endif

int mp_console(char *host)
{
    int result;
    sock_set_t set;
    sock_t sock, insock;
    sock_event_t event;
    SOCK_NATIVE_FD stdin_fd;
    smpd_context *list, *context;
    smpd_context *session_context;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    HANDLE hThread;
    OVERLAPPED ovl;
    HANDLE hWrite;
    char name[100];
    /*DWORD mode;*/
#endif

    /* create a session with the host */
    result = smpd_connect_to_smpd(SOCK_INVALID_SET, SOCK_INVALID_SOCK, host, SMPD_SMPD_SESSION_STR, &set, &sock);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to connect to smpd on %s\n", host);
	return result;
    }

    /* create a context for the session */
    context = (smpd_context*)malloc(sizeof(smpd_context));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context, size %d\n", sizeof(smpd_context));
	return SMPD_FAIL;
    }
    smpd_init_context(context, set, sock);
    strcpy(context->host, host);
    list = context;
    sock_set_user_ptr(sock, context);
    session_context = context;

    /* post a read for a possible incoming command */
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to post a read for an incoming command from the smpd on '%s', error:\n%s\n",
	    context->host, get_sock_error_string(result));
	return SMPD_FAIL;
    }

    /* create a context for reading from stdin */
    context = (smpd_context*)malloc(sizeof(smpd_context));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context, size %d\n", sizeof(smpd_context));
	return SMPD_FAIL;
    }

    /* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
    gen_unique_name(name, 100);
    hWrite = CreateNamedPipe(name, 
	PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
	PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
	1, 0, 0, 0, NULL);
    if (hWrite == INVALID_HANDLE_VALUE)
    {
	mp_err_printf("CreateNamedPipe('%s') failed, error %d\n", name, GetLastError());
	return SMPD_FAIL;
    }
    stdin_fd = CreateFile(name,
	GENERIC_READ | GENERIC_WRITE,
	0,
	NULL,
	OPEN_EXISTING,
	FILE_FLAG_OVERLAPPED,
	NULL);
    if (stdin_fd == INVALID_HANDLE_VALUE)
    {
	mp_err_printf("CreateFile('%s') failed, error %d\n", name, GetLastError());
	return SMPD_FAIL;
    }
    ovl.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ConnectNamedPipe(hWrite, &ovl))
    {
	switch (GetLastError())
	{
	case ERROR_PIPE_LISTENING:
	    mp_dbg_printf("waiting for pipe to connect.\n");
	    WaitForSingleObject(ovl.hEvent, INFINITE);
	    break;
	case ERROR_PIPE_CONNECTED:
	    mp_dbg_printf("ConnectNamedPipe returned immediate success.\n");
	    break;
	default:
	    mp_err_printf("ConnectNamedPipe failed, error %d\n", GetLastError());
	    return SMPD_FAIL;
	    break;
	}
    }
    CloseHandle(ovl.hEvent);
    /* Setting this mode causes the pipe to close:
    mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
    if (!SetNamedPipeHandleState(stdin_fd, &mode, NULL, NULL))
    {
	mp_err_printf("SetNamedPipeHandleState failed, error %d\n", GetLastError());
	return SMPD_FAIL;
    }
    */
#else
    stdin_fd = fileno(stdin);
#endif

    /* convert the native handle to a sock */
    result = sock_native_to_sock(set, stdin_fd, context, &insock);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("unable to create a sock from stdin, sock error:\n%s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }
    smpd_init_context(context, set, insock);
    context->next = list;
    list = context;

#ifdef HAVE_WINDOWS_H
    /* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
       stdin to a sock */
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StdinThread, hWrite, 0, &dwThreadID);
    if (hThread == NULL)
    {
	mp_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
	return SMPD_FAIL;
    }
    CloseHandle(hThread);
#endif

    /* post a read for a user command from stdin */
    result = sock_post_read(insock, context->input_str, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
	    get_sock_error_string(result));
	return SMPD_FAIL;
    }

    /* wait for and handle commands */
    result = SMPD_SUCCESS;
    while (result == SMPD_SUCCESS)
    {
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}

	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    result = handle_read(event.user_ptr, event.num_bytes, event.error, session_context);
	    if (result == SMPD_CLOSE)
	    {
		mp_dbg_printf("handle_read returned SMPD_CLOSE\n");
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
	    mp_err_printf("unexpected close event returned by sock_wait.\n");
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
	return result;
    }
    mp_dbg_printf("mp_console returning SMPD_SUCCESS\n");
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
