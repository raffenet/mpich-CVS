#ifndef MPIDPRE_H
#define MPIDPRE_H

typedef struct MPID_VC
{
    volatile int ref_count;
    int lpid;
    struct MPID_Request * sendq_head;
    struct MPID_Request * sendq_tail;
    struct MPID_Request * recv_active;
} MPID_VC;

typedef struct MM_Car
{
    struct MPID_Request *request_ptr;
    int dest, src;
    struct MM_Car *next_ptr;
} MM_Car;

#define MPID_DEV_REQUEST_DECL \
    int src, dest, tag; \
    MPID_Comm *comm_ptr; \
    const void *sbuf; \
    void *rbuf; \
    int count; \
    MPI_Datatype dtype; \
    int first; \
    int last; \
    MM_Car *write_list; \
    MM_Car *read_list; \
    MM_Car wcar; \
    MM_Car rcar; \
    int op_valid; \
    struct MPID_Request *next_ptr;

#endif
