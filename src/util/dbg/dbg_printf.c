/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int dbg_printf(char *str, ...)
{
    int n;
    va_list list;

    va_start(list, str);
    n = vprintf(str, list);
    va_end(list);

    fflush(stdout);

    return n;
}

void log_error(char *str)
{
}

int err_printf(char *str, ...)
{
    int n;
    va_list list;

    va_start(list, str);
    n = vprintf(str, list);
    va_end(list);

    fflush(stdout);

    log_error(str);

    return n;
}

void log_msg(char *str)
{
}

int msg_printf(char *str, ...)
{
    int n;
    va_list list;

    va_start(list, str);
    n = vprintf(str, list);
    va_end(list);

    fflush(stdout);

    log_msg(str);

    return n;
}
