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

#ifdef HAVE_WINDOWS_H
void smpd_translate_win_error(int error, char *msg, int maxlen, char *prepend, ...)
{
    HLOCAL str;
    int num_bytes;
    int len;
    va_list list;

    num_bytes = FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_ALLOCATE_BUFFER,
	0,
	error,
	MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	(LPTSTR) &str,
	0,0);
    if (prepend == NULL)
	memcpy(msg, str, min(maxlen, num_bytes+1));
    else
    {
	va_start(list, prepend);
	len = vsnprintf(msg, maxlen, prepend, list);
	va_end(list);
	msg += len;
	maxlen -= len;
	snprintf(msg, maxlen, "%s", str);
    }
    LocalFree(str);
}
#endif

char * get_sock_error_string(int error)
{
    static char str[256];

    if (error == MPIDU_SOCK_SUCCESS)
	return "operation completed successfully";

    MPIR_Err_get_string_ext(error, str, 256, MPIDU_Sock_get_error_class_string);
    return str;
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
    case SMPD_CONTEXT_MPIEXEC_STDIN:
	return "mpi_stdin";
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
    if (envstr != NULL)
    {
	if (strstr(envstr, "stdout"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_STDOUT;
	if (strstr(envstr, "log"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_LOGFILE;
	if (strstr(envstr, "rank"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_PREPEND_RANK;
	if (strstr(envstr, "trace"))
	    smpd_process.dbg_state |= SMPD_DBG_STATE_TRACE;
	if (strstr(envstr, "all"))
	    smpd_process.dbg_state = SMPD_DBG_STATE_ALL;
    }

    if (smpd_option_on("log"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_LOGFILE;
    if (smpd_option_on("prepend_rank"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_PREPEND_RANK;
    if (smpd_option_on("trace"))
	smpd_process.dbg_state |= SMPD_DBG_STATE_TRACE;

    if (smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE)
    {
	envstr = getenv("SMPD_DBG_LOG_FILENAME");
	if (envstr)
	{
	    strcpy(smpd_process.dbg_filename, envstr);
	}
	else
	{
	    if (smpd_get_smpd_data("logfile", smpd_process.dbg_filename, SMPD_MAX_FILENAME) != SMPD_SUCCESS)
	    {
		smpd_process.dbg_state ^= SMPD_DBG_STATE_LOGFILE;
	    }
	}
    }

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, SMPD_OUTPUT_MUTEX_NAME);
	smpd_process.bOutputInitialized = TRUE;
    }
#endif
    return SMPD_SUCCESS;
}

int smpd_finalize_printf()
{
    return SMPD_SUCCESS;
}

void smpd_clean_output(char *str)
{
    char *temp_str;

    temp_str = strstr(str, "password");
    while (temp_str != NULL)
    {
	smpd_hide_string_arg(temp_str, "password");
	temp_str = strstr(temp_str+1, "password");
    }

    temp_str = strstr(str, "pwd");
    while (temp_str != NULL)
    {
	smpd_hide_string_arg(temp_str, "pwd");
	temp_str = strstr(temp_str+1, "pwd");
    }

    temp_str = strstr(str, "phrase");
    while (temp_str != NULL)
    {
	smpd_hide_string_arg(temp_str, "phrase");
	temp_str = strstr(temp_str+1, "phrase");
    }
}

int smpd_err_printf(char *str, ...)
{
    va_list list;
    char *indent_str;
    char *cur_str;
    int num_bytes;

    if (!(smpd_process.dbg_state & (SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_LOGFILE)))
	return 0;

    /* write the formatted string to a global buffer */

    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
	indent_str = indent;
    else
	indent_str = "";

    num_bytes = 0;
    if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
    {
	/* prepend output with the process tree node id */
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "[%02d]%sERROR:", smpd_process.id, indent_str);
    }
    else
    {
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "%s", indent_str);
    }
    cur_str = &smpd_process.printf_buffer[num_bytes];

    va_start(list, str);
    num_bytes += vsnprintf(cur_str, SMPD_MAX_DBG_PRINTF_LENGTH - num_bytes, str, list);
    va_end(list);

    /* strip protected fields - passwords, etc */
    smpd_clean_output(smpd_process.printf_buffer);

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, SMPD_OUTPUT_MUTEX_NAME);
	smpd_process.bOutputInitialized = TRUE;
    }
    WaitForSingleObject(smpd_process.hOutputMutex, INFINITE);
#endif


    if (smpd_process.dbg_state & SMPD_DBG_STATE_ERROUT)
    {
	/* use stdout instead of stderr so that ordering will be consistent with dbg messages */
	printf("%s", smpd_process.printf_buffer);
	fflush(stdout);
    }
    if ((smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE) && (smpd_process.dbg_filename[0] != '\0'))
    {
	FILE *fout;
	fout = fopen(smpd_process.dbg_filename, "a+");
	if (fout == NULL)
	{
	    smpd_process.dbg_state ^= SMPD_DBG_STATE_LOGFILE;
	}
	else
	{
	    fprintf(fout, "%s", smpd_process.printf_buffer);
	    fclose(fout);
	}
    }
    

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(smpd_process.hOutputMutex);
#endif
    return num_bytes;
}

int smpd_dbg_printf(char *str, ...)
{
    va_list list;
    char *indent_str;
    char *cur_str;
    int num_bytes;

    if (!(smpd_process.dbg_state & (SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_LOGFILE)))
	return 0;

    /* write the formatted string to a global buffer */

    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
	indent_str = indent;
    else
	indent_str = "";

    num_bytes = 0;
    if (smpd_process.dbg_state & SMPD_DBG_STATE_PREPEND_RANK)
    {
	/* prepend output with the process tree node id */
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "[%02d]%s", smpd_process.id, indent_str);
    }
    else
    {
	num_bytes = snprintf(smpd_process.printf_buffer, SMPD_MAX_DBG_PRINTF_LENGTH, "%s", indent_str);
    }
    cur_str = &smpd_process.printf_buffer[num_bytes];

    va_start(list, str);
    num_bytes += vsnprintf(cur_str, SMPD_MAX_DBG_PRINTF_LENGTH - num_bytes, str, list);
    va_end(list);

    /* strip protected fields - passwords, etc */
    smpd_clean_output(smpd_process.printf_buffer);

#ifdef HAVE_WINDOWS_H
    if (!smpd_process.bOutputInitialized)
    {
	smpd_process.hOutputMutex = CreateMutex(NULL, FALSE, SMPD_OUTPUT_MUTEX_NAME);
	smpd_process.bOutputInitialized = TRUE;
    }
    WaitForSingleObject(smpd_process.hOutputMutex, INFINITE);
#endif

    if (smpd_process.dbg_state & SMPD_DBG_STATE_STDOUT)
    {
	printf("%s", smpd_process.printf_buffer);
	fflush(stdout);
    }
    if ((smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE) && (smpd_process.dbg_filename[0] != '\0'))
    {
	FILE *fout;
	fout = fopen(smpd_process.dbg_filename, "a+");
	if (fout == NULL)
	{
	    smpd_process.dbg_state ^= SMPD_DBG_STATE_LOGFILE;
	}
	else
	{
	    fprintf(fout, "%s", smpd_process.printf_buffer);
	    fclose(fout);
	}
    }
    

#ifdef HAVE_WINDOWS_H
    ReleaseMutex(smpd_process.hOutputMutex);
#endif
    return num_bytes;
}

int smpd_enter_fn(char *fcname)
{
    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
    {
	smpd_dbg_printf("\\%s\n", fcname);
    }
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
    if (smpd_process.dbg_state & SMPD_DBG_STATE_TRACE)
    {
	smpd_dbg_printf("/%s\n", fcname);
    }
    return SMPD_SUCCESS;
}

SMPD_BOOL smpd_snprintf_update(char **str_pptr, int *len_ptr, char *str_format, ...)
{
    va_list list;
    int n;

    va_start(list, str_format);
    n = vsnprintf(*str_pptr, *len_ptr, str_format, list);
    va_end(list);

    if (n < 0)
    {
	(*str_pptr)[(*len_ptr)-1] = '\0';
	*len_ptr = 0;
	return SMPD_FALSE;
    }

    (*str_pptr )= &(*str_pptr)[n];
    *len_ptr = (*len_ptr) - n;

    return SMPD_TRUE;
}
