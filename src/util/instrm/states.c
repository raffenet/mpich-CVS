/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

void MPID_TimerStateBegin( int id )
{
#if HAVE_TIMING 
    MPICH_PerThread_t *p;
    MPID_GetPerThread( p );
    p->timestamps[id].count++;
#endif
}
void MPID_TimerStateEnd( int id )
{
#if HAVE_TIMING 
    MPICH_PerThread_t *p;
    MPID_GetPerThread( p );
/*     p->timestamps[id].stamp += 0; */
#endif
}
