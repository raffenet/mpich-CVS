/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIIMPLTHREAD_H
#define MPIIMPLTHREAD_H

/* Thread basics */
#ifdef MPICH_SINGLE_THREADED
typedef int MPID_Thread_key_t;
typedef int MPID_Thread_id_t;
typedef int MPID_Thread_lock_t;
#define MPID_Thread_lock( a )
#define MPID_Thread_unlock( a )
#define MPID_Thread_lock_init( a )
#define MPID_GetPerThread(p) p = &MPIR_Thread
#else /* Assumes pthreads for simplicity */
/* Eventually replace this with an include of a header file with the 
   correct definitions for the specified thread package. */
#if defined HAVE_PTHREAD_CREATE
#include <pthread.h>
typedef pthread_key_t MPID_Thread_key_t;
typedef pthread_t MPID_Thread_id_t;
typedef pthread_mutex_t MPID_Thread_lock_t;
#define MPID_GetPerThread(p) {\
     p = (MPICH_PerThread_t*)pthread_getspecific( MPIR_Process.thread_key ); \
     if (!p) { p = MPIU_Calloc( 1, sizeof(MPICH_PerThread_t ) );\
               pthread_setspecific( MPIR_Process.thread_key, p );}}
#define MPID_Thread_lock( a ) pthread_mutex_lock( a )
#define MPID_Thread_unlock( a ) pthread_mutex_unlock( a )
#define MPID_Thread_lock_init( a ) pthread_mutex_init( a, 0 )
#else
#error No Thread Package Chosen
#endif
#endif

#endif
