/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#include <stdio.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_WINDOWS_H
static BOOL bInitialized = FALSE;
static HANDLE hOutputMutex = NULL;
#endif

char * get_sock_error_string(int error)
{
    static char str[256];
    switch (error)
    {
    case SOCK_FAIL:
	return "generic sock failure";
    case SOCK_EOF:
	return "end of file found on socket";
    case SOCK_ERR_NOMEM:
	return "not enough memory to complete the socket operation";
    case SOCK_ERR_TIMEOUT:
	return "socket operation timed out";
    case SOCK_ERR_HOST_LOOKUP:
	return "host lookup failed";
    case SOCK_ERR_CONN_REFUSED:
	return "socket connection refused";
    case SOCK_ERR_CONN_FAILED:
	return "socket connection failed";
    case SOCK_ERR_BAD_SOCK:
	return "invalid socket";
    case SOCK_ERR_BAD_BUFFER:
	return "invalid buffer";
    case SOCK_ERR_OP_IN_PROGRESS:
	return "socket operation in progress";
    case SOCK_ERR_OS_SPECIFIC:
	return "operating system specific socket error occurred";
    default:
	sprintf(str, "unknown socket error %d", error);
	return str;
    }
    return NULL;
}

int smpd_err_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

#ifdef HAVE_WINDOWS_H
    if (!bInitialized)
    {
	hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	bInitialized = TRUE;
    }
    WaitForSingleObject(hOutputMutex, INFINITE);
#endif

    /* use stdout instead of stderr so that ordering will be consistent with dbg messages */

    /* prepend output with the process tree node id */
    fprintf(stdout, "[%d]ERROR:", smpd_process.id);

    /* print the formatted string */
    va_start(list, str);
    format_str = str;
    n = vfprintf(stdout, format_str, list);
    va_end(list);

    fflush(stdout);

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(hOutputMutex);
#endif
    return n;
}

int smpd_dbg_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

#ifdef HAVE_WINDOWS_H
    if (!bInitialized)
    {
	hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	bInitialized = TRUE;
    }
    WaitForSingleObject(hOutputMutex, INFINITE);
#endif

    /* prepend output with the tree node id */
    printf("[%d]", smpd_process.id);

    /* print the formatted string */
    va_start(list, str);
    format_str = str;
    n = vfprintf(stdout, format_str, list);
    va_end(list);

    fflush(stdout);

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(hOutputMutex);
#endif

    return n;
}

#define SMPD_MAX_INDENT 20
static char indent[SMPD_MAX_INDENT+1] = "";
static int cur_indent = 0;

int smpd_enter_fn(char *fcname)
{
#ifdef HAVE_WINDOWS_H
    if (!bInitialized)
    {
	hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	bInitialized = TRUE;
    }
    WaitForSingleObject(hOutputMutex, INFINITE);
#endif

    printf("[%d]%sentering %s\n", smpd_process.id, indent, fcname);
    fflush(stdout);
    if (cur_indent >= 0 && cur_indent < SMPD_MAX_INDENT)
    {
	indent[cur_indent] = '.';
	indent[cur_indent+1] = '\0';
    }
    cur_indent++;

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(hOutputMutex);
#endif

    return SMPD_SUCCESS;
}

int smpd_exit_fn(char *fcname)
{
#ifdef HAVE_WINDOWS_H
    if (!bInitialized)
    {
	hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	bInitialized = TRUE;
    }
    WaitForSingleObject(hOutputMutex, INFINITE);
#endif

    if (cur_indent > 0 && cur_indent < SMPD_MAX_INDENT)
    {
	indent[cur_indent-1] = '\0';
    }
    cur_indent--;
    printf("[%d]%sexiting %s\n", smpd_process.id, indent, fcname);
    fflush(stdout);

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(hOutputMutex);
#endif

    return SMPD_SUCCESS;
}
