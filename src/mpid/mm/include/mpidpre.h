#ifndef MPIDPRE_H
#define MPIDPRE_H

#include "mpi.h"
#include "mpidconf.h"

#ifdef HAVE_NEW_METHOD
#include "new_method_pre.h"
#endif

enum MM_CAR_TYPE { 
    MM_UNBOUND_WRITE_CAR,
    MM_UNPACK_CAR,
#ifdef WITH_METHOD_SHM
    MM_SHM_CAR, 
#endif
#ifdef WITH_METHOD_TCP
    MM_TCP_CAR, 
#endif
#ifdef WITH_METHOD_VIA
    MM_VIA_CAR, 
#endif
#ifdef WITH_METHOD_VIA_RDMA
    MM_VIA_RDMA_CAR 
#endif
#ifdef WITH_METHOD_NEW
    MM_NEW_METHOD_CAR
#endif
};

typedef struct MM_Car
{
    struct MPID_Request *request_ptr;
    struct MPIDI_VC *vc_ptr;
    int dest;
    enum MM_CAR_TYPE type;
    union {
	int dummy;
#ifdef WITH_METHOD_SHM
#endif
#ifdef WITH_METHOD_TCP
#endif
#ifdef WITH_METHOD_VIA
#endif
#ifdef WITH_METHOD_VIA_RDMA
#endif
#ifdef HAVE_NEW_METHOD
#ifdef WITH_METHOD_NEW
	MPID_DEV_METHOD_NEW_DECL
#endif
#endif
    } data;
    struct MM_Car *next_ptr;
} MM_Car;

typedef struct MM_Segment
{
    int src, tag;
    const void *sbuf;
    void *rbuf;
    int count;
    MPI_Datatype dtype;
    int first;
    int last;
    MM_Car *write_list;
    MM_Car wcar;
    MM_Car rcar;
    int op_valid;
    struct MPID_Request *next_ptr;
} MM_Segment;

#define MPID_DEV_REQUEST_DECL MM_Segment mm;

#endif
