#include "mpidu_process_locks.h"

#ifdef USE_PROCESS_LOCKS

#include "mpidu_process_locks.h"
#include <errno.h>
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

int g_nLockSpinCount = 100;

#ifndef USE_BUSY_LOCKS
/*@
   MPIDU_Process_lock_init - 

   Parameters:
+  MPIDU_Process_lock_t *lock

   Notes:
@*/
void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_NT_LOCKS
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    *lock = CreateMutex(NULL, FALSE, NULL);
    if (*lock == NULL)
    {
        printf("error in mutex_init: %d\n", GetLastError());
    }
#elif defined(HAVE_PTHREAD_H)
    /* should be called by one process only */
    int err;
    pthread_mutexattr_t attr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
#ifdef HAVE_PTHREAD_MUTEXATTR_INIT
    err = pthread_mutexattr_init(&attr);
    if (err != 0)
      printf("error in pthread_mutexattr_init: %s\n", strerror(err));
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_SETPSHARED
    err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (err != 0)
      printf("error in pthread_mutexattr_setpshared: %s\n", strerror(err));

    err = pthread_mutex_init( lock, &attr );
#else
    err = pthread_mutex_init( lock, NULL );
#endif
    if ( err != 0 ) 
        printf( "error in mutex_init: %s\n", strerror(err) );
#else
#error Locking functions not defined
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

/*@
   MPIDU_Process_lock - 

   Parameters:
+  MPIDU_Process_lock_t *lock

   Notes:
@*/
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_NT_LOCKS
    DWORD dwRetVal;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    /*printf("nt lock %x\n", lock);fflush(stdout);*/
    dwRetVal = WaitForSingleObject(*lock, INFINITE);
    if (dwRetVal != WAIT_OBJECT_0)
    {
        if (dwRetVal == WAIT_FAILED)
            printf("error in mutex_lock: %s\n", strerror(GetLastError()));
        else
            printf("error in mutex_lock: %d\n", GetLastError());
    }
    /*printf("lock: Handle = %u\n", (unsigned long)*lock);*/
#elif defined(HAVE_PTHREAD_H)
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    err = pthread_mutex_lock( lock );
    if ( err != 0 ) 
        printf( "error in mutex_lock: %s\n", strerror(err) );
#else
#error Locking functions not defined
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
}

/*@
   MPIDU_Process_unlock - 

   Parameters:
+  MPIDU_Process_lock_t *lock

   Notes:
@*/
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_NT_LOCKS
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    if (!ReleaseMutex(*lock))
    {
        printf("error in mutex_unlock: %d\n", GetLastError());
        printf("Handle = %u\n", (unsigned long)*lock);
    }
    /*printf("unlock: Handle = %u\n", (unsigned long)*lock);*/
#elif defined(HAVE_PTHREAD_H)
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    err = pthread_mutex_unlock( lock );
    if ( err != 0 ) 
        printf( "error in mutex_unlock: %s\n", strerror(err) );
#else
#error Locking functions not defined
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

/*@
   MPIDU_Process_lock_busy_wait - 

   Parameters:
+  MPIDU_Process_lock_t *lock

   Notes:
@*/
void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    MPIDU_Process_lock(lock);
    MPIDU_Process_unlock(lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
}

/*@
   MPIDU_Process_lock_free - 

   Parameters:
+  MPIDU_Process_lock_t *lock

   Notes:
@*/
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
#ifdef HAVE_NT_LOCKS
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    /*printf("Free_lock: Handle = %u\n", (unsigned long)*lock);*/
    CloseHandle(*lock);
    *lock = NULL;
#elif defined(HAVE_PTHREAD_H)
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    err = pthread_mutex_destroy( lock );
    if ( err != 0 ) 
    printf( "error in mutex_destroy: %s\n", strerror(err) );
#else
#error Locking functions not defined
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}
#endif /* #ifndef USE_BUSY_LOCKS */

#endif /* USE_PROCESS_LOCKS */
