/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * Test performance of atomic access operations.
 * This is a *very* simple test.
 */

#include "mpiimpl.h"

#define MPID_Atomic_incr( count_ptr ) \
   __asm__ __volatile__ ( "lock; inc %1" \
                         : "a" (count_ptr) )
                      
int main( int argc, char **argv )
{
    int                i, n;
    MPID_Thread_lock_t mutex;
    MPID_Time_t        start_t, end_t;
    double             time_lock, time_incr;

    /* Set values */
    n = 1000;

    /* Test atomic increment using lock/unlock */
    count = 0;
    MPID_Thread_lock_init( &mutex );
    MPID_Wtime( &start_t );
    for (i=0; i<n; i++) {
	MPID_Thread_lock( &mutex );
	count ++;
	MPID_Thread_unlock( &mutex );
    }
    MPID_Wtime( &end_t );
    MPID_Wtime_diff( &start_t, &end_t, &time_lock );
    time_lock /= n;
    if (count != n) {
	printf( "Error in thread-locked atomic update\n" );
    }

    /* Test atomic increment using special instructions */
    count = 0;
    MPID_Wtime( &start_t );
    for (i=0; i<n; i++) {
	MPID_Atomic_incr( &count );
    }
    MPID_Wtime( &end_t );
    MPID_Wtime_diff( &start_t, &end_t, &time_incr );
    time_incr /= n;
    if (count != n) {
	printf( "Error in native atomic update\n" );
    }

    
    return 0;
}
