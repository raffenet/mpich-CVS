/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmutilconf.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>

/*
 * Provide a simple timeout capability.  Initialize the time with 
 * InitTimeout.  Call GetRemainingTime to get the time in seconds left.
 */
static int end_time = -1;  /* Time of timeout in seconds */

void InitTimeout( int seconds )
{
#ifdef HAVE_TIME
    time_t t;
    t = time( NULL );
    end_time = seconds + t;
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval tp;
    gettimeofday( &tp, NULL );
    end_time = seconds + tp.tv_sec;
#else
#   error 'No timer available'
#endif
}

/* Return remaining time in seconds */
int GetRemainingTime( void )
{
    int time_left;
#ifdef HAVE_TIME
    time_t t;
    t = time( NULL );
    time_left = end_time - t;
#elif defined(HAVE_GETTIMEOFDAY)
    struct timeval tp;
    gettimeofday( &tp, NULL );
    time_left = end_time - tp.tv_sec;
#else
#   error 'No timer available'
#endif
    if (time_left < 0) time_left = 0;
    return time_left;
}

