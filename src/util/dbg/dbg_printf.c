/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file provides a set of routines that can be used to record debug
 * messages in a ring so that the may be dumped at a later time.  For example,
 * this can be used to record debug messages without printing them; when
 * a special event, such as an error occurs, a call to 
 * MPIU_dump_dbg_memlog( stderr ) will print the contents of the file ring
 * to stderr.
 */
#include "mpiimpl.h"
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

/* Temporary.  sig values will change */
/* style: allow:vprintf:3 sig:0 */
/* style: allow:fputs:1 sig:0 */
/* style: allow:printf:2 sig:0 */

#ifdef HAVE_VA_COPY
# define va_copy_end(a) va_end(a)
#else
# ifdef HAVE___VA_COPY
#  define va_copy(a,b) __va_copy(a,b)
#  define va_copy_end(a) 
# else
#  define va_copy(a,b) ((a) = (b))
/* Some writers recommend define va_copy(a,b) memcpy(&a,&b,sizeof(va_list)) */
#  define va_copy_end(a)
# endif
#endif

#if !defined(MPICH_DBG_MEMLOG_NUM_LINES)
#define MPICH_DBG_MEMLOG_NUM_LINES 1024
#endif
#if !defined(MPICH_DBG_MEMLOG_LINE_SIZE)
#define MPICH_DBG_MEMLOG_LINE_SIZE 256
#endif

MPIU_dbg_state_t MPIUI_dbg_state = MPIU_DBG_STATE_UNINIT;
FILE * MPIUI_dbg_fp = NULL;
static int dbg_memlog_num_lines = MPICH_DBG_MEMLOG_NUM_LINES;
static int dbg_memlog_line_size = MPICH_DBG_MEMLOG_LINE_SIZE;
static char **dbg_memlog = NULL;
static int dbg_memlog_next = 0;
static int dbg_memlog_count = 0;
static int dbg_rank = -1;

static void dbg_init(void);

int MPIU_dbg_init(int rank)
{
    dbg_rank = rank;

    if (MPIUI_dbg_state == MPIU_DBG_STATE_UNINIT)
    {
	dbg_init();
    }

    /* If file logging is enable, we need to open a file */
    if (MPIUI_dbg_state & MPIU_DBG_STATE_FILE)
    {
	char fn[128];

	/* Only open the file only once in case MPIU_dbg_init is called more than once */
	if (MPIUI_dbg_fp == NULL)
	{
	    sprintf(fn, "mpich2-dbg-%d.log", dbg_rank);
	    MPIUI_dbg_fp = fopen(fn, "w");
	    setvbuf(MPIUI_dbg_fp, NULL, _IONBF, 0);
	}
    }
    
    return 0;
}

static void dbg_init(void)
{
    char * envstr;
    
    MPIUI_dbg_state = 0;

    /* FIXME: This should use MPIU_Param_get_string */
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
    if (strstr(envstr, "stdout"))
    {
	MPIUI_dbg_state |= MPIU_DBG_STATE_STDOUT;
    }
    if (strstr(envstr, "memlog"))
    {
	MPIUI_dbg_state |= MPIU_DBG_STATE_MEMLOG;
    }
    if (strstr(envstr, "file"))
    {
	MPIUI_dbg_state |= MPIU_DBG_STATE_FILE;
    }

    /* If memlog is enabled, the we need to allocate some memory for it */
    if (MPIUI_dbg_state & MPIU_DBG_STATE_MEMLOG)
    {
	dbg_memlog = MPIU_Malloc(dbg_memlog_num_lines * sizeof(char *) + dbg_memlog_num_lines * dbg_memlog_line_size);
	if (dbg_memlog != NULL)
	{
	    int i;
	    
	    for (i = 0; i < dbg_memlog_num_lines ; i++)
	    {
		dbg_memlog[i] = ((char *) &dbg_memlog[dbg_memlog_num_lines]) + i * dbg_memlog_line_size;
	    }
	}
	else
	{
	    MPIUI_dbg_state &= ~MPIU_DBG_STATE_MEMLOG;
	}
    }
}

int MPIU_dbglog_printf(const char *str, ...)
{
    int n = 0;
    va_list list;

    if (MPIUI_dbg_state == MPIU_DBG_STATE_UNINIT)
    {
	dbg_init();
    }

    if (MPIUI_dbg_state & MPIU_DBG_STATE_MEMLOG)
    {
	/* FIXME: put everything on one line until a \n is found */
	
	dbg_memlog[dbg_memlog_next][0] = '\0';
	va_start(list, str);
	n = vsnprintf(dbg_memlog[dbg_memlog_next], dbg_memlog_line_size, str, list);
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
	    n = (int)strlen(dbg_memlog[dbg_memlog_next]);
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
    }

    if ((MPIUI_dbg_state & MPIU_DBG_STATE_FILE) && MPIUI_dbg_fp != NULL)
    {
	va_start(list, str);
	n = vfprintf(MPIUI_dbg_fp, str, list);
	va_end(list);
    }

    return n;
}

int MPIU_dbglog_vprintf(const char *str, va_list ap)
{
    int n = 0;
    va_list list;

    if (MPIUI_dbg_state == MPIU_DBG_STATE_UNINIT)
    {
	dbg_init();
    }

    if (MPIUI_dbg_state & MPIU_DBG_STATE_MEMLOG)
    {
	va_copy(list,ap);
	dbg_memlog[dbg_memlog_next][0] = '\0';
	n = vsnprintf(dbg_memlog[dbg_memlog_next], dbg_memlog_line_size, str, list);
        va_copy_end(list);

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
	    n = (int)strlen(dbg_memlog[dbg_memlog_next]);
	}

	if (dbg_memlog[dbg_memlog_next][0] != '\0')
	{
	    dbg_memlog_next = (dbg_memlog_next + 1) % dbg_memlog_num_lines;
	    dbg_memlog_count++;
	}
    }

    if (MPIUI_dbg_state & MPIU_DBG_STATE_STDOUT)
    {
	va_copy(list, ap);
	n = vprintf(str, list);
	va_copy_end(list);
    }

    if ((MPIUI_dbg_state & MPIU_DBG_STATE_FILE) && MPIUI_dbg_fp != NULL)
    {
	va_copy(list, ap);
	n = vfprintf(MPIUI_dbg_fp, str, list);
	va_end(list);
    }

    return n;
}

int MPIU_dbg_printf(const char * str, ...)
{
    int n;
    
    MPID_Common_thread_lock();
    {
	va_list list;

	MPIU_dbglog_printf("[%d]", dbg_rank);
	va_start(list, str);
	n = MPIU_dbglog_vprintf(str, list);
	va_end(list);
	MPIU_dbglog_flush();
    }
    MPID_Common_thread_unlock();
    
    return n;
}

void MPIU_dump_dbg_memlog_to_stdout(void)
{
    MPIU_dump_dbg_memlog(stdout);
}

void MPIU_dump_dbg_memlog_to_file(const char *filename)
{
    FILE *fout;
    fout = fopen(filename, "wb");
    if (fout != NULL)
    {
	MPIU_dump_dbg_memlog(fout);
	fclose(fout);
    }
}

void MPIU_dump_dbg_memlog(FILE * fp)
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
	fflush(fp);
    }
}

