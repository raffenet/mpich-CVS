/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIIMPLTHREAD_H
#define MPIIMPLTHREAD_H

#define MPICH_THREAD_LEVEL_SINGLE 1
#define MPICH_THREAD_LEVEL_FUNNELED 2
#define MPICH_THREAD_LEVEL_SERIAL 3
#define MPICH_THREAD_LEVEL_MULTIPLE 4

#define MPICH_THREAD_IMPL_NOT_IMPLEMENTED -1
#define MPICH_THREAD_IMPL_NONE 1
#define MPICH_THREAD_IMPL_GLOBAL_MUTEX 2
#define MPICH_THREAD_IMPL_GLOBAL_MONITOR 3

#define MPICH_THREAD_PKG_UNSUPPORTED -1
#define MPICH_THREAD_PKG_NONE 1
#define MPICH_THREAD_PKG_POSIX 2
#define MPICH_THREAD_PKG_SOLARIS 3

/* Thread basics */
#ifdef MPICH_SINGLE_THREADED
typedef int MPID_Thread_key_t;
typedef int MPID_Thread_id_t;
typedef int MPID_Thread_lock_t;
typedef int MPID_Thread_cond_t;

#define MPID_GetPerThread(p) p = &MPIR_Thread

#define MPID_Thread_lock_init(mutexp_)
#define MPID_Thread_lock(mutexp_)
#define MPID_Thread_unlock(mutexp_)
#define MPID_Thread_lock_destroy(mutexp_)

#define MPID_Thread_cond_init(condp_)
#define MPID_Thread_cond_signal(condp_)
#define MPID_Thread_cond_wait(condp_, mutexp_)
#define MPID_Thread_cond_destroy(mutexp_)

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

#define MPID_Thread_lock_init(mutexp_) pthread_mutex_init((mutexp_), NULL)
#define MPID_Thread_lock(mutexp_) pthread_mutex_lock(mutexp_)
#define MPID_Thread_unlock(mutexp_) pthread_mutex_unlock(mutexp_)
#define MPID_Thread_lock_destroy(mutexp_) pthread_mutex_destroy(mutexp_)

#define MPID_Thread_cond_init(condp_) pthread_cond_init((condp_), NULL)
#define MPID_Thread_cond_signal(condp_) pthread_cond_signal(condp_)
#define MPID_Thread_cond_wait(condp_, mutexp_) pthread_cond_wait((condp_),(mutexp_))
#define MPID_Thread_cond_destroy(condp_) pthread_cond_destroy(condp_)

#elif HAVE_THR_CREATE
#include <thread.h>
typedef thread_key_t MPID_Thread_key_t;
typedef thread_t MPID_Thread_id_t;
typedef mutex_t MPID_Thread_lock_t;

#define MPID_GetPerThread(p_)					\
{								\
    thr_getspecific(MPIR_Process.thread_key, &(p_));		\
    if ((p_) == NULL)						\
    {								\
	(p_) = MPIU_Calloc(1, sizeof(MPICH_PerThread_t));	\
	thr_setspecific(MPIR_Process.thread_key, (p_));		\
    }								\
}

#define MPID_Thread_lock_init(mutexp_) mutex_init((mutexp_), NULL)
#define MPID_Thread_lock(mutexp_) mutex_lock(mutexp_)
#define MPID_Thread_unlock(mutexp_) mutex_unlock(mutexp_)
#define MPID_Thread_lock_destroy(mutexp_) mutex_destroy(mutexp_)

#else
#error No Thread Package Chosen
#endif

#if defined(HAVE_THR_YIELD)
#define MPID_Thread_yield() thr_yield()
#elif defined(HAVE_SCHED_YIELD)
#define MPID_Thread_yield() sched_yield()
#elif defined(HAVE_YIELD)
#define MPID_Thread_yield() yield()
#endif

#endif /* defined(MPICH_SINGLE_THREADED) */

void MPID_Thread_key_create(MPID_Thread_key_t *);
MPID_Thread_id_t MPID_Thread_get_id(void);

#endif /* defined(MPIIMPLTHREAD_H) */
