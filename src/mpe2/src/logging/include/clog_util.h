/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#if !defined( _CLOG_UTIL )
#define _CLOG_UTIL

#include "clog_common.h"

void CLOG_Util_abort( int errorcode );

char* CLOG_Util_get_tmpfilename( void );

void CLOG_Util_swap_bytes( void *bytes, int elem_sz, int Nelem );

char *CLOG_Util_strbuf_put(       char *buf_ptr, const char *buf_tail,
                            const char *val_str, const char *err_str );

char *CLOG_Util_strbuf_get(       char *val_ptr, const char *val_tail,
                            const char *buf_str, const char *err_str );

int CLOG_Util_is_MPIWtime_synchronized( void );
#endif  /* of _CLOG_UTIL */
