#include <errno.h>
#include <pthread.h>

typedef pthread_mutex_t MPE_Thread_mutex_t;
typedef pthread_cond_t  MPE_Thread_cond_t;
typedef pthread_t       MPE_Thread_id_t;
typedef pthread_key_t   MPE_Thread_tls_t;
