#ifndef _DLOGIMPL
#define _DLOGIMPL

#if defined(NEEDS_STDLIB_PROTOTYPES) && !defined ( malloc )
#include "protofix.h"
#endif

#include "dlog.h"

#if defined(MPIR_MEMDEBUG)
/* Enable memory tracing.  This requires MPICH's mpid/util/tr2.c codes */
#include "mpimem.h"		/* Chameleon memory debugging stuff */
#define MALLOC(a)    MPID_trmalloc((unsigned)(a),__LINE__,__FILE__)
#define FREE(a)      MPID_trfree(a,__LINE__,__FILE__)
#else
#define MALLOC(a)    malloc(a)
#define FREE(a)      free(a)
#define MPID_trvalid(a)
#endif


void DLOG_dumplog ( void );
void DLOG_outblock (double *);
void DLOG_dumpblock ( double * );
int  DLOG_reclen ( int );
void DLOG_msgtype ( int );
void DLOG_commtype ( int );
void DLOG_colltype ( int );
void DLOG_rectype ( int );

#endif



