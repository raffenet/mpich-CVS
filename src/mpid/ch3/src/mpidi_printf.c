/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include <stdio.h>
#include <stdarg.h>

/* style: allow:vprintf:1 sig:0 */
/* style: allow:printf:2 sig:0 */

#undef MPIDI_dbg_printf
void MPIDI_dbg_printf(int level, char * func, char * fmt, ...)
{
    va_list list;
    
    MPIU_dbglog_printf("[%d] %s(): ", MPIR_Process.comm_world->rank, func);
    va_start(list, fmt);
    MPIU_dbglog_vprintf(fmt, list);
    va_end(list);
    MPIU_dbglog_printf("\n");
    fflush(stdout);
}

#undef MPIDI_err_printf
void MPIDI_err_printf(char * func, char * fmt, ...)
{
    va_list list;
    
    printf("[%d] ERROR - %s(): ", MPIR_Process.comm_world->rank, func);
    va_start(list, fmt);
    vprintf(fmt, list);
    va_end(list);
    printf("\n");
    fflush(stdout);
}
