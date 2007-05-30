#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
#if defined( HAVE_PTHREAD_H )
#include <pthread.h>
#endif

#if defined( HAVE_LIBPTHREAD )

#define MPE_ThreadID_t  int

/* MPE coarse-grained lock support mechanism */
pthread_mutex_t  MPE_Thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_key_t    MPE_ThreadID_key;
MPE_ThreadID_t   MPE_Thread_count = 0;

void MPE_ThreadID_free( void *thdID_ptr );
void MPE_ThreadID_free( void *thdID_ptr )
{
    if ( thdID_ptr != NULL ) {
        free( thdID_ptr );
        thdID_ptr = NULL;
    }
}

void MPE_Log_thread_init( void );
void MPE_Log_thread_init( void )
{
    int   thd_fn_rc;
    MPE_Thread_count = 0;
    thd_fn_rc = pthread_key_create( &MPE_ThreadID_key, MPE_ThreadID_free );
    if ( thd_fn_rc != 0 ) {
        perror( "pthread_key_create() fails!" );
        pthread_exit( NULL );
    }
}

#else

void MPE_ThreadID_free( void *thdID_ptr );
void MPE_ThreadID_free( void *thdID_ptr )
{}

void MPE_Log_thread_init( void );
void MPE_Log_thread_init( void )
{}

#endif
