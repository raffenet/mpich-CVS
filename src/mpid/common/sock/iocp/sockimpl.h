#include <winsock2.h>

#define SOCK_IOV             WSABUF
#define SOCK_IOV_LEN         len
#define SOCK_IOV_BUF         buf
#define SOCK_IOV_MAXLEN      16
#define SOCK_INFINITE        INFINITE
#define SOCK_INVALID_SOCKET  INVALID_SOCKET
#define SOCK_SIZE_MAX	     INT_MAX

typedef HANDLE sock_set_t;
typedef struct sock_state_t * sock_t;
typedef int sock_size_t;
