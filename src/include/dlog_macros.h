/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DLOG_MACROS_H
#define DLOG_MACROS_H

#include "dlog.h"

/* structures, global variables */
#define MPID_MAX_COLOR_STR 40
typedef struct MPID_Timer_state
{
    int in_id, out_id;
    int num_calls;
    unsigned long color;
    char color_str[MPID_MAX_COLOR_STR];
    char *name;
} MPID_Timer_state;

extern MPID_Timer_state g_timer_state[MPID_NUM_TIMER_STATES];
extern DLOG_Struct *g_pDLOG;

/* state declaration macros */
#define MPIDU_STATE_DECL(a)
#define MPIDU_INIT_STATE_DECL(a)
#define MPIDU_FINALIZE_STATE_DECL(a)

/* function enter and exit macros */
#define MPIDU_FUNC_ENTER(a) \
    DLOG_LogOpenEvent( \
	g_pDLOG, \
	g_timer_state[ a ].in_id, \
	g_timer_state[ a ].num_calls++ )

#define MPIDU_FUNC_EXIT(a) \
    DLOG_LogOpenEvent( \
	g_pDLOG, \
	g_timer_state[ a ].out_id, \
	g_timer_state[ a ].num_calls++ )

#define MPIDU_PT2PT_FUNC_ENTER(a)     MPIDU_FUNC_ENTER(a)
#define MPIDU_PT2PT_FUNC_EXIT(a)      MPIDU_FUNC_EXIT(a)
#define MPIDU_COLL_FUNC_ENTER(a)      MPIDU_FUNC_ENTER(a)
#define MPIDU_COLL_FUNC_EXIT(a)       MPIDU_FUNC_EXIT(a)
#define MPIDU_RMA_FUNC_ENTER(a)       MPIDU_FUNC_ENTER(a)
#define MPIDU_RMA_FUNC_EXIT(a)        MPIDU_FUNC_EXIT(a)
#define MPIDU_INIT_FUNC_ENTER(a)
#define MPIDU_INIT_FUNC_EXIT(a)
#define MPIDU_FINALIZE_FUNC_ENTER(a)
#define MPIDU_FINALIZE_FUNC_EXIT(a)

/* arrow generating enter and exit macros */
#ifdef MPID_LOG_ARROWS
#define MPIDU_PT2PT_FUNC_ENTER_FRONT(a) \
{ \
    DLOG_LogOpenEvent( g_pDLOG, g_timer_state[ a ].in_id, g_timer_state[ a ].num_calls++ ); \
    DLOG_LogSend( g_pDLOG, dest, tag, count ); \
}
#define MPIDU_PT2PT_FUNC_ENTER_BACK(a) \
{ \
    DLOG_LogOpenEvent( g_pDLOG, g_timer_state[ a ].in_id, g_timer_state[ a ].num_calls++ ); \
    DLOG_LogRecv( g_pDLOG, source, tag, count ); \
}
#define MPIDU_PT2PT_FUNC_ENTER_BOTH(a) \
{ \
    DLOG_LogOpenEvent( g_pDLOG, g_timer_state[ a ].in_id, g_timer_state[ a ].num_calls++ ); \
    DLOG_LogSend( g_pDLOG, dest, sendtag, sendcount ); \
    DLOG_LogRecv( g_pDLOG, source, recvtag, recvcount ); \
}
#else
#define MPIDU_PT2PT_FUNC_ENTER_FRONT MPIDU_FUNC_ENTER
#define MPIDU_PT2PT_FUNC_ENTER_BACK  MPIDU_FUNC_ENTER
#define MPIDU_PT2PT_FUNC_ENTER_BOTH  MPIDU_FUNC_ENTER
#endif

/* function prototypes */
/*
int MPIU_Timer_init(int rank, int size);
int MPIU_Timer_finalize();
*/



/**********************************/
#ifdef FOO
#define MPID_STATE_DECLS double time_stamp
#define MPID_FUNC_ENTER(a) time_stamp = DLOG_timestamp()
#define MPID_FUNC_EXIT(a)  DLOG_LogEvent( g_pDLOG, a, time_stamp )

#define MPID_MPI_STATE_DECLS          MPID_STATE_DECLS
#define MPID_MPI_FINALIZE_STATE_DECLS
#define MPID_MPI_INIT_STATE_DECLS
#define MPID_MPI_FUNC_ENTER           MPID_FUNC_ENTER
#define MPID_MPI_FUNC_EXIT            MPID_FUNC_EXIT

#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a) \
{ \
    time_stamp = DLOG_timestamp(); \
    DLOG_LogSend( g_pDLOG, dest, tag, count ); \
}
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a) \
{ \
    time_stamp = DLOG_timestamp(); \
    DLOG_LogRecv( g_pDLOG, source, tag, count ); \
}
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a) \
{ \
    time_stamp = DLOG_timestamp(); \
    DLOG_LogSend( g_pDLOG, dest, tag, count ); \
    DLOG_LogRecv( g_pDLOG, source, tag, count ); \
}

#define MPID_MPI_PT2PT_FUNC_ENTER        MPID_MPI_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_EXIT         MPID_MPI_FUNC_EXIT
#define MPID_MPI_COLL_FUNC_ENTER         MPID_MPI_FUNC_ENTER
#define MPID_MPI_COLL_FUNC_EXIT          MPID_MPI_FUNC_EXIT
#define MPID_MPI_INIT_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a) 

int MPIU_Timer_init(int rank, int size);
int MPIU_Timer_finalize();
#endif
/**********************************/

#endif
