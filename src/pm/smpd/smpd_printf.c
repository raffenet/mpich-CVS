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

#define SMPD_MAX_INDENT 20
static char indent[SMPD_MAX_INDENT+1] = "";
static int cur_indent = 0;

char * get_sock_error_string(int error)
{
    static char str[256];
    switch (error)
    {
    case SOCK_SUCCESS:
	return "operation completed successfully";
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
    case SOCK_ERR_OP_ABORTED:
	return "socket operation aborted";
    case SOCK_ERR_OS_SPECIFIC:
	sprintf(str, "operating system specific socket error %d occurred", sock_get_last_os_error());
	return str;
    default:
	sprintf(str, "unknown socket error %d", error);
	return str;
    }
    return NULL;
}

char * smpd_get_context_str(smpd_context_t *context)
{
    if (context == NULL)
	return "null";
    switch (context->type)
    {
    case SMPD_CONTEXT_INVALID:
	return "invalid";
    case SMPD_CONTEXT_STDIN:
	return "stdin";
    case SMPD_CONTEXT_STDOUT:
	return "stdout";
    case SMPD_CONTEXT_STDERR:
	return "stderr";
    case SMPD_CONTEXT_PARENT:
	return "parent";
    case SMPD_CONTEXT_LEFT_CHILD:
	return "left";
    case SMPD_CONTEXT_RIGHT_CHILD:
	return "right";
    case SMPD_CONTEXT_CHILD:
	return "child";
    case SMPD_CONTEXT_LISTENER:
	return "listener";
    case SMPD_CONTEXT_SMPD:
	return "smpd";
    case SMPD_CONTEXT_PMI:
	return "pmi";
    case SMPD_CONTEXT_UNDETERMINED:
	return "undetermined";
    case SMPD_CONTEXT_FREED:
	return "freed";
    }
    return "unknown";
}

int smpd_init_printf(void)
{
    char * envstr;

    smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT;

    envstr = getenv("SMPD_DBG_OUTPUT");
    if (envstr == NULL)
    {
	return SMPD_SUCCESS;
    }

    if (strstr(envstr, "stdout"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_STDOUT;
    if (strstr(envstr, "log"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_LOGFILE;
    if (strstr(envstr, "rank"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_PREPEND_RANK;

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	smpd_process.bOutputInitialized = TRUE;
    }
#endif
    return SMPD_SUCCESS;
}

int smpd_err_printf(char *str, ...)
{
    int n = 0;
    va_list list;
    char *format_str;

    if (smpd_process.dbg_state == 0)
	return 0;

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	smpd_process.bOutputInitialized = TRUE;
    }
    WaitForSingleObject(smpd_process.hOutputMutex, INFINITE);
#endif

    va_start(list, str);

    if (smpd_process.dbg_state & SMPD_DBG_STATE_ERROUT)
    {
	/* use stdout instead of stderr so that ordering will be consistent with dbg messages */

	if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
	{
	    /* prepend output with the process tree node id */
	    fprintf(stdout, "[%02d]%sERROR:", smpd_process.id, indent);
	}
	else
	{
	    fprintf(stdout, "%s", indent);
	}

	/* print the formatted string */
	format_str = str;
	n = vfprintf(stdout, format_str, list);

	fflush(stdout);
    }
    if (smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE)
    {
	if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
	{
	    /* prepend output with the process tree node id */
	    fprintf(smpd_process.dbg_fout, "[%02d]%sERROR:", smpd_process.id, indent);
	}
	else
	{
	    fprintf(smpd_process.dbg_fout, "%sERROR:", indent);
	}

	/* print the formatted string */
	format_str = str;
	n = vfprintf(smpd_process.dbg_fout, format_str, list);

	fflush(smpd_process.dbg_fout);
    }

    va_end(list);

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(smpd_process.hOutputMutex);
#endif
    return n;
}

int smpd_dbg_printf(char *str, ...)
{
    int n = 0;
    va_list list;
    char *format_str;

    if (smpd_process.dbg_state == SMPD_DBG_STATE_ERROUT)
	return 0;

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, "SMPD_OUTPUT_MUTEX");
	smpd_process.bOutputInitialized = TRUE;
    }
    WaitForSingleObject(smpd_process.hOutputMutex, INFINITE);
#endif

    va_start(list, str);

    if (smpd_process.dbg_state & SMPD_DBG_STATE_STDOUT)
    {
	if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
	{
	    /* prepend output with the tree node id */
	    printf("[%02d]%s", smpd_process.id, indent);
	}
	else
	{
	    printf("%s", indent);
	}

	/* print the formatted string */
	format_str = str;
	n = vfprintf(stdout, format_str, list);

	fflush(stdout);
    }
    if (smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE)
    {
	if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
	{
	    /* prepend output with the process tree node id */
	    fprintf(smpd_process.dbg_fout, "[%02d]%s", smpd_process.id, indent);
	}
	else
	{
	    fprintf(smpd_process.dbg_fout, "%s", indent);
	}

	/* print the formatted string */
	format_str = str;
	n = vfprintf(smpd_process.dbg_fout, format_str, list);

	fflush(smpd_process.dbg_fout);
    }

    va_end(list);

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(smpd_process.hOutputMutex);
#endif

    return n;
}

int smpd_enter_fn(char *fcname)
{
    smpd_dbg_printf("\\%s\n", fcname);
    if (cur_indent >= 0 && cur_indent < SMPD_MAX_INDENT)
    {
	indent[cur_indent] = '.';
	indent[cur_indent+1] = '\0';
    }
    cur_indent++;
    return SMPD_SUCCESS;
}

int smpd_exit_fn(char *fcname)
{
    if (cur_indent > 0 && cur_indent < SMPD_MAX_INDENT)
    {
	indent[cur_indent-1] = '\0';
    }
    cur_indent--;
    smpd_dbg_printf("/%s\n", fcname);
    return SMPD_SUCCESS;
}
