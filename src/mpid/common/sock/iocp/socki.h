#include <winsock2.h>

#define SOCK_IOV             WSABUF
#define SOCK_IOV_LEN         len
#define SOCK_IOV_BUF         buf
#define SOCK_IOV_MAXLEN      16
#define SOCK_INFINITE_TIME   INFINITE
#define SOCK_INVALID_SOCK    NULL
#define SOCK_SIZE_MAX	     INT_MAX

typedef HANDLE sock_set_t;
typedef struct sock_state_t * sock_t;
typedef int sock_size_t;

#define inline __inline

int Sock_describe_timer_states();
int Socki_describe_timer_states();

#define SOCKI_STATE_LIST \
MPID_STATE_SOCK_EASY_RECEIVE, \
MPID_STATE_SOCK_EASY_SEND, \
MPID_STATE_GETQUEUEDCOMPLETIONSTATUS,
