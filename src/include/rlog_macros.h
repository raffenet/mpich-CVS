#ifndef RLOG_MACROS_H
#define RLOG_MACROS_H

#include "rlog.h"

extern RLOG_Struct *g_pRLOG;

#define MPID_STATE_DECL(a) double time_stamp_##a
#define MPID_FUNC_ENTER(a) g_pRLOG->nRecursion++; time_stamp_##a = RLOG_timestamp()
#define MPID_FUNC_EXIT(a)  RLOG_LogEvent( g_pRLOG, a, time_stamp_##a, RLOG_timestamp(), --g_pRLOG->nRecursion )

#define MPID_MPI_STATE_DECL          MPID_STATE_DECL
#define MPID_MPI_FINALIZE_STATE_DECL
#define MPID_MPI_INIT_STATE_DECL
#define MPID_MPI_FUNC_ENTER           MPID_FUNC_ENTER
#define MPID_MPI_FUNC_EXIT            MPID_FUNC_EXIT

#define LOG_ARROWS
#ifdef LOG_ARROWS
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a) \
{ \
    g_pRLOG->nRecursion++; \
    time_stamp_##a = RLOG_timestamp(); \
    RLOG_LogSend( g_pRLOG, dest, tag, count ); \
}
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a) \
{ \
    g_pRLOG->nRecursion++; \
    time_stamp_##a = RLOG_timestamp(); \
    RLOG_LogRecv( g_pRLOG, source, tag, count ); \
}
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a) \
{ \
    g_pRLOG->nRecursion++; \
    time_stamp_##a = RLOG_timestamp(); \
    RLOG_LogSend( g_pRLOG, dest, sendtag, sendcount ); \
    RLOG_LogRecv( g_pRLOG, source, recvtag, recvcount ); \
}
#else
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT MPID_MPI_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK  MPID_MPI_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH  MPID_MPI_FUNC_ENTER
#endif

#define MPID_MPI_PT2PT_FUNC_ENTER        MPID_MPI_FUNC_ENTER
#define MPID_MPI_PT2PT_FUNC_EXIT         MPID_MPI_FUNC_EXIT
#define MPID_MPI_COLL_FUNC_ENTER         MPID_MPI_FUNC_ENTER
#define MPID_MPI_COLL_FUNC_EXIT          MPID_MPI_FUNC_EXIT
#define MPID_MPI_INIT_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a) 

#endif
