/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#if defined(USE_GETTEXT)
#include <libintl.h>
#endif

/* style: allow:vprintf:2 sig:0 */

int MPIU_Usage_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vprintf(format_str, list);
    va_end(list);

    fflush(stdout);

    return n;
}

int MPIU_Error_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vfprintf(stderr, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

int MPIU_Internal_error_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vfprintf(stderr, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

int MPIU_Msg_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    va_start(list, str);
#ifdef USE_GETTEXT
    /* Category is LC_MESSAGES */
    format_str = dgettext( "mpich", str );
    if (!format_str) format_str = str;
#else
    format_str = str;
#endif
    n = vfprintf(stdout, format_str, list);
    va_end(list);

    fflush(stderr);

    return n;
}

