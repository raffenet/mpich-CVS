#ifndef RLOG_MACROS_H
#define RLOG_MACROS_H

#include "rlog.h"

extern RLOG_Struct *g_pRLOG;

#define MPIDU_STATE_DECL(a) double time_stamp_##a
#define MPIDU_INIT_STATE_DECL(a)
#define MPIDU_FINALIZE_STATE_DECL(a)

#define MPIDU_FUNC_ENTER(a) if (g_pRLOG) { g_pRLOG->nRecursion++; time_stamp_##a = RLOG_timestamp(); }
#define MPIDU_FUNC_EXIT(a)  if (g_pRLOG) { RLOG_LogEvent( g_pRLOG, a, time_stamp_##a, RLOG_timestamp(), --g_pRLOG->nRecursion ); }

#define MPIDU_PT2PT_FUNC_ENTER(a)     MPIDU_FUNC_ENTER(a)
#define MPIDU_PT2PT_FUNC_EXIT(a)      MPIDU_FUNC_EXIT(a)
#define MPIDU_COLL_FUNC_ENTER(a)      MPIDU_FUNC_ENTER(a)
#define MPIDU_COLL_FUNC_EXIT(a)       MPIDU_FUNC_EXIT(a)
#define MPIDU_INIT_FUNC_ENTER(a)
#define MPIDU_INIT_FUNC_EXIT(a)
#define MPIDU_FINALIZE_FUNC_ENTER(a)
#define MPIDU_FINALIZE_FUNC_EXIT(a)

#define MPIDU_PT2PT_FUNC_ENTER_FRONT(a) if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    time_stamp_##a = RLOG_timestamp(); \
    RLOG_LogSend( g_pRLOG, dest, tag, count ); \
}
#define MPIDU_PT2PT_FUNC_ENTER_BACK(a) if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    time_stamp_##a = RLOG_timestamp(); \
    RLOG_LogRecv( g_pRLOG, source, tag, count ); \
}
#define MPIDU_PT2PT_FUNC_ENTER_BOTH(a) if (g_pRLOG) \
{ \
    g_pRLOG->nRecursion++; \
    time_stamp_##a = RLOG_timestamp(); \
    RLOG_LogSend( g_pRLOG, dest, sendtag, sendcount ); \
    RLOG_LogRecv( g_pRLOG, source, recvtag, recvcount ); \
}

#endif
