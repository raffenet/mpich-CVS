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

#endif
