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
#define MPIR_GetPerThread(pt_)						\
{									\
    MPID_Thread_tls_get(&MPIR_Process.thread_storage, (pt_));		\
    if (*(pt_) == NULL)							\
    {									\
	*(pt_) = (MPICH_PerThread_t *) MPIU_Calloc(1, sizeof(MPICH_PerThread_t));	\
	MPID_Thread_tls_set(&MPIR_Process.thread_storage, (void *) *(pt_));\
    }									\
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

/*
 * New Thread Interface and Macros
 * See http://www-unix.mcs.anl.gov/mpi/mpich2/developer/design/threads.htm
 * for a detailed discussion of this interface and the rationale.
 * 
 * This code implements the single CS approach, and is similar to 
 * the MPID_CS_ENTER and MPID_CS_EXIT macros above.  
 *
 */

/* Make the CPP definition that will be used to control whether threaded 
   code is supported.  Test ONLY on whether MPICH_IS_THREADED is defined.
*/
#if !defined(MPICH_THREAD_LEVEL) || !defined(MPI_THREAD_MULTIPLE)
#error Internal error in macro definitions in include/mpiimplthread.h
#endif

#if (MPICH_THREAD_LEVEL == MPI_THREAD_MULTIPLE)
#define MPICH_IS_THREADED 1
#endif

#ifdef MPICH_IS_THREADED
#ifdef HAVE_RUNTIME_THREADCHECK
#define MPIU_THREAD_CHECK_BEGIN if (MPIR_Process.isThreaded) {
#define MPIU_THREAD_CHECK_END   }  
#else
#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END
#endif

/* SINGLE_CS_DECL needs to take over the decl used by MPID_CS_xxx when that
   is removed */
#define MPIU_THREAD_SINGLE_CS_DECL
#define MPIU_THREAD_SINGLE_CS_INITIALIZE MPID_CS_INITIALIZE()
#define MPIU_THREAD_SINGLE_CS_FINALIZE   MPID_CS_FINALIZE()
#define MPIU_THREAD_SINGLE_CS_ASSERT_WITHIN(_msg)
#define MPIU_THREAD_SINGLE_CS_ENTER(_msg) MPIU_THREAD_CHECK_BEGIN MPID_CS_ENTER() MPIU_THREAD_CHECK_END
#define MPIU_THREAD_SINGLE_CS_EXIT(_msg)  MPIU_THREAD_CHECK_BEGIN MPID_CS_EXIT() MPIU_THREAD_CHECK_END

/* These provide a uniform way to perform a first-use initialization
   in a thread-safe way.  See the web page or wtime.c */
#define MPIU_THREADSAFE_INIT_DECL(_var) static volatile int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) \
     if (_var) { \
      MPIU_THREAD_SINGLE_CS_ENTER(""); \
      _stmt; _var=0; \
      MPIU_THREAD_SINGLE_CS_EXIT(""); \
     }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var) \
     MPIU_THREAD_SINGLE_CS_ENTER(""); \
     if (_var) {
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var) \
      } \
     MPIU_THREAD_SINGLE_CS_EXIT("")

#else
/* MPICH user routines are single-threaded */
#define MPIU_THREAD_SINGLE_CS_DECL
#define MPIU_THREAD_SINGLE_CS_INITIALIZE
#define MPIU_THREAD_SINGLE_CS_FINALIZE
#define MPIU_THREAD_SINGLE_CS_ASSERT_WITHIN(_msg)
#define MPIU_THREAD_SINGLE_CS_ENTER(_msg)
#define MPIU_THREAD_SINGLE_CS_EXIT(_msg)
#define MPIU_THREAD_CHECK_BEGIN
#define MPIU_THREAD_CHECK_END

/* These provide a uniform way to perform a first-use initialization
   in a thread-safe way.  See the web page or mpidtime.c for the generic
   wtick */
#define MPIU_THREADSAFE_INIT_DECL(_var) static int _var=1
#define MPIU_THREADSAFE_INIT_STMT(_var,_stmt) if (_var) { _stmt; _var = 0; }
#define MPIU_THREADSAFE_INIT_BLOCK_BEGIN(_var)
#define MPIU_THREADSAFE_INIT_CLEAR(_var) _var=0
#define MPIU_THREADSAFE_INIT_BLOCK_END(_var)
#endif

#endif /* !defined(MPIIMPLTHREAD_H_INCLUDED) */
