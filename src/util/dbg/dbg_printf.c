/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#if !defined(MPICH_DBG_MEMLOG_NUM_LINES)
#define MPICH_DBG_MEMLOG_NUM_LINES 1024
#endif
#if !defined(MPICH_DBG_MEMLOG_LINE_SIZE)
#define MPICH_DBG_MEMLOG_LINE_SIZE 256
#endif

MPIU_dbg_state_t MPIUI_dbg_state = MPIU_DBG_STATE_UNINIT;
static int dbg_memlog_num_lines = MPICH_DBG_MEMLOG_NUM_LINES;
static int dbg_memlog_line_size = MPICH_DBG_MEMLOG_LINE_SIZE;
static char **dbg_memlog = NULL;
static int dbg_memlog_next = 0;
static int dbg_memlog_count = 0;

static void dbg_init(void)
{
    char * envstr;
    
    MPIUI_dbg_state = 0;
    
    envstr = getenv("MPICH_DBG_OUTPUT");
    if (envstr == NULL)
    {
	return;
    }

    /*
     * TODO:
     *
     * - parse environment variable to determine number of log lines, etc.
     *
     * - add support for writing to a (per-process or global?) file
     *
     * - add support for sending to a log server, perhaps with global time
     *   sequencing information ???
     */
    if (strcmp(envstr, "memlog") == 0)
    {
	MPIUI_dbg_state = MPIU_DBG_STATE_MEMLOG;
    }
    else if (strcmp(envstr, "stdout") == 0)
    {
	MPIUI_dbg_state = MPIU_DBG_STATE_STDOUT;
    }
    else if (strcmp(envstr, "memlog,stdout") == 0 ||
	     strcmp(envstr, "stdout,memlog") == 0)
    {
	MPIUI_dbg_state = MPIU_DBG_STATE_STDOUT | MPIU_DBG_STATE_MEMLOG;
    }

    /* If memlog is enabled, the we need to allocate some memory for it */
    if (MPIUI_dbg_state & MPIU_DBG_STATE_MEMLOG)
    {
	dbg_memlog = MPIU_Malloc(dbg_memlog_num_lines * sizeof(char *) +
				 dbg_memlog_num_lines * dbg_memlog_line_size);
	if (dbg_memlog != NULL)
	{
	    int i;
	    
	    for (i = 0; i < dbg_memlog_num_lines ; i++)
	    {
		dbg_memlog[i] = ((char *) &dbg_memlog[dbg_memlog_num_lines]) +
		    i * dbg_memlog_line_size;
	    }
	}
	else
	{
	    MPIUI_dbg_state &= ~MPIU_DBG_STATE_MEMLOG;
	}
    }
}

int MPIU_dbg_printf(char *str, ...)
{
    int n = 0;
    va_list list;

    if (MPIUI_dbg_state == MPIU_DBG_STATE_UNINIT)
    {
	dbg_init();
    }

    if (MPIUI_dbg_state & MPIU_DBG_STATE_MEMLOG)
    {
	dbg_memlog[dbg_memlog_next][0] = '\0';
	va_start(list, str);
	n = vsnprintf(dbg_memlog[dbg_memlog_next], dbg_memlog_line_size,
		      str, list);
	va_end(list);

	/* if the output was truncated, we null terminate the end of the
	   string, on the off chance that vsnprintf() didn't do that.  we also
	   check to see if any data has been written over the null we set at
	   the beginning of the string.  this is mostly paranoia, but the man
	   page does not clearly state what happens when truncation occurs.  if
	   data was written to the string, we would like to output it, but we
	   want to avoid reading past the end of the array or outputing garbage
	   data. */

	if (n < 0 || n >= dbg_memlog_line_size)
	{
	    dbg_memlog[dbg_memlog_next][dbg_memlog_line_size - 1] = '\0';
	    n = strlen(dbg_memlog[dbg_memlog_next]);
	}

	if (dbg_memlog[dbg_memlog_next][0] != '\0')
	{
	    dbg_memlog_next = (dbg_memlog_next + 1) % dbg_memlog_num_lines;
	    dbg_memlog_count++;
	}
    }

    if (MPIUI_dbg_state & MPIU_DBG_STATE_STDOUT)
    {
	va_start(list, str);
	n = vprintf(str, list);
	va_end(list);
    
	fflush(stdout);
    }

    return n;
}


void log_error(char *str)
{
}

#undef err_printf
int err_printf(char *str, ...)
{
    int n;
    va_list list;

    va_start(list, str);
    n = vprintf(str, list);
    va_end(list);

    fflush(stdout);

    log_error(str);

    exit(-1);

    return n;
}

void log_msg(char *str)
{
}

#undef msg_printf
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

void MPIU_dump_dbg_memlog_to_stdout(void)
{
    MPIU_dump_dbg_memlog_to_file(stdout);
}

void MPIU_dump_dbg_memlog_to_file(FILE * fp)
{
    if (dbg_memlog_count != 0)
    {
	int ent;
	int last_ent;

	/* there is a small issue with counter rollover which will need to be
	   fixed if more than 2^32 lines are going to be logged */
	ent = (dbg_memlog_next == dbg_memlog_count) ? 0 : dbg_memlog_next;
	last_ent = (ent + dbg_memlog_num_lines - 1) % dbg_memlog_num_lines;
	
	do
	{
	    fputs(dbg_memlog[ent], fp);
	    ent = (ent + 1) % dbg_memlog_num_lines;
	}
	while(ent != last_ent);
    }
}

