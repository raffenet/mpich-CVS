/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#include <stdio.h>
#include <stdarg.h>

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

    va_start(list, str);
    format_str = str;
    n = vfprintf(stderr, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

int smpd_dbg_printf(char *str, ...)
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
