#ifndef MPIDU_PROCESS_LOCKS_H
#define MPIDU_PROCESS_LOCKS_H

#include <stdio.h>

extern int g_nLockSpinCount;

#ifdef HAVE_YIELD
#define MPIDU_Yield() yield()
#elif defined(HAVE_WIN32_SLEEP)
#define MPIDU_Yield() Sleep(0)
#elif defined (HAVE_SCHED_YIELD)
#define MPIDU_Yield() sched_yield()
#elif defined (HAVE_SELECT)
#define MPIDU_Yield() { struct timeval t; t.tv_sec = 0; t.tv_usec = 0; select(0,0,0,0,&t); }
#elif defined (HAVE_USLEEP)
#define MPIDU_Yield() usleep(0)
#elif defined (HAVE_SLEEP)
#define MPIDU_Yield() sleep(0)
#else
#error *** No yield function specified ***
#endif

#ifdef HAVE_MUTEX_INIT
/*   Only known system is Solaris */
#include <sys/systeminfo.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <synch.h>
#include <string.h>

typedef mutex_t                 MPIDU_Process_lock_t;
#define MPIDU_Process_lock_init(lock)   mutex_init(lock,USYNC_PROCESS,(void *)NULL)
#define MPIDU_Process_lock(lock)        mutex_lock(lock)
#define MPIDU_Process_unlock(lock)      mutex_unlock(lock)
#define MPIDU_Process_lock_free(lock)   mutex_destroy(lock)
static inline void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    int i;
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    mutex_lock(lock);
    mutex_unlock(lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
}

#else

#ifdef USE_BUSY_LOCKS
#ifdef HAVE_MUTEX_INIT
typedef mutex_t MPIDU_Process_lock_t;
#else
typedef volatile long MPIDU_Process_lock_t;
#endif
#else
#ifdef HAVE_NT_LOCKS
typedef HANDLE MPIDU_Process_lock_t;
#elif defined(HAVE_PTHREAD_H)
typedef pthread_mutex_t MPIDU_Process_lock_t;  
#else
#error *** No locking mechanism for shared memory.specified ***
#endif
#endif

#include <errno.h>
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef USE_BUSY_LOCKS

static inline void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_MUTEX_INIT
    int err;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
#ifdef HAVE_MUTEX_INIT
    memset(lock, 0, sizeof(MPIDU_Process_lock_t));
    err = mutex_init(lock, USYNC_PROCESS, 0);
    if (err)
      printf("mutex_init error: %d\n", err);
#else
    *(lock) = 0;
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

static inline void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_MUTEX_INIT
    int err;
    err = mutex_lock(lock);
    if (err)
      printf("mutex_lock error: %d\n", err);
#else
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
        {
            if (*lock == 0)
            {
#ifdef HAVE_INTERLOCKEDEXCHANGE
                if (InterlockedExchange((LPLONG)lock, 1) == 0)
                {
                    /*printf("lock %x\n", lock);fflush(stdout);*/
                    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
                    return;
                }
#elif defined(HAVE_COMPARE_AND_SWAP)
                if (compare_and_swap(lock, 0, 1) == 1)
                {
                    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
                    return;
                }
#else
#error *** No atomic memory operation specified to implement busy locks ***
#endif
            }
        }
        MPIDU_Yield();
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
#endif
}

static inline void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_MUTEX_INIT
    int err;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
#ifdef HAVE_MUTEX_INIT
    err = mutex_lock(lock);
    if (err)
      printf("mutex_unlock error: %d\n", err);
#else
    *(lock) = 0;
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

static inline void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock )
{
    int i;
#ifdef HAVE_MUTEX_INIT
    int err;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
#ifdef HAVE_MUTEX_INIT
    err = mutex_lock(lock);
    if (err)
      printf("mutex_lock error: %d\n", err);
    err = mutex_unlock(lock);
    if (err)
      printf("mutex_unlock error: %d\n", err);
#else
    for (;;)
    {
        for (i=0; i<g_nLockSpinCount; i++)
            if (!*lock)
            {
		MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
                return;
            }
        MPIDU_Yield();
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
}

static inline void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_MUTEX_INIT
    int err;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
#ifdef HAVE_MUTEX_INIT
    err = mutex_destroy(lock);
    if (err)
      printf("mutex_destroy error: %d\n", err);
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}

#else

void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock );
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock );
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock );
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock );
void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock );

#endif /* #ifdef USE_BUSY_LOCKS */
#endif /* #ifdef HAVE_MUTEX_INIT */

/*@
   MPIDU_Compare_swap - 

   Parameters:
+  void **dest
.  void *new_val
.  void *compare_val
.  MPIDU_Process_lock_t *lock
-  void **original_val

   Notes:
@*/
static inline int MPIDU_Compare_swap( void **dest, void *new_val, void *compare_val,            
                        MPIDU_Process_lock_t *lock, void **original_val )
{
    /* dest = pointer to value to be checked (address size)
       new_val = value to set dest to if *dest == compare_val
       original_val = value of dest prior to this operation */

    MPIDI_STATE_DECL(MPID_STATE_MPIDU_COMPARE_SWAP);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_COMPARE_SWAP);
#ifdef HAVE_NT_LOCKS
    /**original_val = InterlockedCompareExchange(dest, new_val, compare_val);*/
    *original_val = (void*)InterlockedCompareExchange((LONG*)dest, (LONG)new_val, (LONG)compare_val);
#elif defined(HAVE_COMPARE_AND_SWAP)
    if (compare_and_swap((volatile long *)dest, (long)compare_val, (long)new_val))
        *original_val = new_val;
#elif defined(HAVE_PTHREAD_H) || defined(HAVE_MUTEX_INIT)
    MPIDU_Process_lock( lock );

    *original_val = *dest;
    
    if ( *dest == compare_val )
        *dest = new_val;

    MPIDU_Process_unlock( lock );
#else
#error *** No locking functions specified ***
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_COMPARE_SWAP);
    return 0;
}

#endif
