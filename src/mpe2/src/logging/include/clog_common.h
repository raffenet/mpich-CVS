/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_COMMON )
#define _CLOG_COMMON

#include "mpe_logging_conf.h"

#if defined( NEEDS_STDLIB_PROTOTYPES ) && !defined ( malloc )
#include "protofix.h"
#endif

#if defined(MPIR_MEMDEBUG)
/* Enable memory tracing.  This requires MPICH's mpid/util/tr2.c codes */
#include "mpimem.h"             /* Chameleon memory debugging stuff */
#define MALLOC(a)    MPID_trmalloc((unsigned)(a),__LINE__,__FILE__)
#define FREE(a)      MPID_trfree(a,__LINE__,__FILE__)
#else
#define MALLOC(a)    malloc(a)
#define FREE(a)      free(a)
#define MPID_trvalid(a)
#endif

/* CLOG_FILE_TYPE determines default CLOG2 file extension, i.e. ".clog2" */
#define  CLOG_FILE_TYPE      "clog2"

#define  CLOG_BOOL_NULL       -1
#define  CLOG_BOOL_FALSE       0
#define  CLOG_BOOL_TRUE        1

#define  CLOG_PATH_STRLEN    256

#define  CLOG_PROCID_NULL     -1

#endif /* of _CLOG_COMMON */
