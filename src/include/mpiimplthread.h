/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIIMPLTHREAD_H_INCLUDED)
#define MPIIMPLTHREAD_H_INCLUDED

#if (MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED)    
#include "mpid_thread.h"
#endif

/*
 * Define possible thread implementations that could be selected at configure time
 */
#define MPICH_THREAD_IMPL_NOT_IMPLEMENTED -1
#define MPICH_THREAD_IMPL_NONE 1
#define MPICH_THREAD_IMPL_GLOBAL_MUTEX 2
#define MPICH_THREAD_IMPL_GLOBAL_MONITOR 3


/*
 * Get a pointer to the thread's private data
 */
#if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
#define MPIR_GetPerThread(pt_)			\
{						\
    *(pt_) = &MPIR_Thread;			\
}
#else
#define MPIR_GetPerThread(pt_)								\
{											\
    MPID_Thread_tls_get(&MPIR_Process.thread_storage, (void **) (pt_));			\
    if (*(pt_) == NULL)									\
    {											\
	*(pt_) = (MPICH_PerThread_t *) MPIU_Calloc(1, sizeof(MPICH_PerThread_t));	\
	MPID_Thread_tls_set(&MPIR_Process.thread_storage, (void *) *(pt_));		\
    }											\
/*printf( "perthread storage (key = %x) is %p\n", MPIR_Process.thread_storage,*pt_); fflush(stdout);*/\
}
#endif


/*
 * Define MPID Critical Section macros, unless the device will be defining them
 */
#if !defined(MPID_DEFINES_MPID_CS)
#if (MPICH_THREAD_LEVEL != MPI_THREAD_MULTIPLE)
#define MPID_CS_INITIALIZE()
#define MPID_CS_FINALIZE()
#define MPID_CS_ENTER()
#define MPID_CS_EXIT()
#elif (USE_THREAD_IMPL == MPICH_THREAD_IMPL_GLOBAL_MUTEX)
/* FIXME: The "thread storage" needs to be moved out of this */
#define MPID_CS_INITIALIZE()						\
{									\
    MPID_Thread_mutex_create(&MPIR_Process.global_mutex, NULL);		\
    MPID_Thread_tls_create(NULL, &MPIR_Process.thread_storage, NULL);   \
}
#define MPID_CS_FINALIZE()						\
{									\
    MPID_Thread_tls_destroy(&MPIR_Process.thread_storage, NULL);	\
    MPID_Thread_mutex_destroy(&MPIR_Process.global_mutex, NULL);	\
}
/* FIXME: Figure out what we want to do for the nest count on 
   these routines, so as to avoid extra function calls */
#define MPID_CS_ENTER()						\
{								\
    MPIU_THREADPRIV_DECL;                                       \
    MPIU_THREADPRIV_GET;                                        \
    if (MPIR_Nest_value() == 0)					\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Enter global critical section");\
	MPID_Thread_mutex_lock(&MPIR_Process.global_mutex);	\
    }								\
}
#define MPID_CS_EXIT()						\
{								\
    MPIU_THREADPRIV_DECL;                                       \
    MPIU_THREADPRIV_GET;                                        \
    if (MPIR_Nest_value() == 0)					\
    { 								\
        MPIU_DBG_MSG(THREAD,TYPICAL,"Exit global critical section");\
	MPID_Thread_mutex_unlock(&MPIR_Process.global_mutex);	\
    }								\
}
#else
#error "Critical section macros not defined"
#endif
#endif /* !defined(MPID_DEFINES_MPID_CS) */


#if defined(HAVE_THR_YIELD)
#undef MPID_Thread_yield
#define MPID_Thread_yield() thr_yield()
#elif defined(HAVE_SCHED_YIELD)
#undef MPID_Thread_yield
#define MPID_Thread_yield() sched_yield()
#elif defined(HAVE_YIELD)
#undef MPID_Thread_yield
#define MPID_Thread_yield() yield()
#endif

#endif /* !defined(MPIIMPLTHREAD_H_INCLUDED) */
