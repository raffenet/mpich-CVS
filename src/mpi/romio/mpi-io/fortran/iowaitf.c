/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpio_wait_ PMPIO_WAIT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpio_wait_ pmpio_wait__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpio_wait pmpio_wait_
#endif
#define mpio_wait_ pmpio_wait
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpio_wait_ pmpio_wait
#endif
#define mpio_wait_ pmpio_wait_
#endif
#else
#ifdef FORTRANCAPS
#define mpio_wait_ MPIO_WAIT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpio_wait_ mpio_wait__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpio_wait mpio_wait_
#endif
#define mpio_wait_ mpio_wait
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpio_wait_ mpio_wait
#endif
#endif
#endif

void mpio_wait_(MPI_Fint *request,MPI_Status *status, int *__ierr )
{
    MPIO_Request req_c;
    
    req_c = MPIO_Request_f2c(*request);
    *__ierr = MPIO_Wait(&req_c, status);
    *request = MPIO_Request_c2f(req_c);
}
