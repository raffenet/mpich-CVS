typedef struct {
    int fd;
    int rdwr;
    int (*handler)( int, int, void * );
    void *extra_data;
} IOHandle;

#define IO_READ  0x1
#define IO_WRITE 0x2

int MPIE_IORegister( int, int, int (*)(int,int,void*), void * );
int MPIE_IOLoop( int );
