/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"

#ifdef __MPIO_BUILD_PROFILING
#ifdef FORTRANCAPS
#define mpio_test_ PMPIO_TEST
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpio_test_ pmpio_test__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpio_test pmpio_test_
#endif
#define mpio_test_ pmpio_test
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpio_test_ pmpio_test
#endif
#define mpio_test_ pmpio_test_
#endif
#else
#ifdef FORTRANCAPS
#define mpio_test_ MPIO_TEST
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpio_test_ mpio_test__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpio_test mpio_test_
#endif
#define mpio_test_ mpio_test
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpio_test_ mpio_test
#endif
#endif
#endif

void mpio_test_(MPI_Fint *request,int *flag,MPI_Status *status, int *__ierr )
{
    MPIO_Request req_c;
    
    req_c = MPIO_Request_f2c(*request);
    *__ierr = MPIO_Test(&req_c,flag,status);
    *request = MPIO_Request_c2f(req_c);
}
