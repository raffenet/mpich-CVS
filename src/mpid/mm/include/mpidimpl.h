#ifndef MPIDIMPL_H
#define MPIDIMPL_H

#include "mpiimpl.h"

#include "bsocket.h"

/* key used by spawners and spawnees to get the port by which they can connect to each other */
#define MPICH_PARENT_PORT_KEY     "MPI_Parent_port"
/* key used to tell comm_accept that it doesn't need to transfer pmi databases */
#define MPICH_PMI_SAME_DOMAIN_KEY "PMI_SAME_DOMAIN"
/* key used to inform spawned processes that their parent is mpiexec and not another mpi application */
#define MPICH_EXEC_IS_PARENT_KEY  "MPIEXECSpawned"

typedef struct OpenPortNode {
    char port_name[MPI_MAX_PORT_NAME];
    int bfd;
    struct OpenPortNode *next;
} OpenPortNode_t;

typedef struct {
    MPID_Thread_lock_t lock;
    char              pmi_kvsname[100];
    MPID_Comm         *comm_parent;
    OpenPortNode_t    *port_list;
} MPID_PerProcess_t;
extern MPID_PerProcess_t MPID_Process;

typedef struct MPIDI_VC
{
    volatile int ref_count;
    struct MM_Car * writeq_head;
    struct MM_Car * writeq_tail;
    struct MM_Car * recvq;
} MPIDI_VC;

typedef struct MM_VCTABLE
{
    volatile int ref_count;
    MPIDI_VC *table_ptr;
} MM_VCTABLE;

int mm_open_port(MPID_Info *, char *);
int mm_close_port(char *);
int mm_accept(MPID_Info *, char *);
int mm_connect(MPID_Info *, char *);
int mm_send(int, char *, int);
int mm_recv(int, char *, int);
int mm_close(int);
MPID_Request * mm_request_alloc();
void mm_request_free(MPID_Request *request_ptr);
void mm_car_init();
void mm_car_finalize();
MM_Car* mm_car_alloc();
void mm_car_free(MM_Car *car_ptr);
int mm_choose_buffer(MPID_Request *request_ptr);
MPIDI_VC *mm_get_vc(MPID_Comm *comm_ptr, int dest);
void mm_vctable_init();
void mm_vctable_finalize();
int mm_alloc_vc_table(MPID_Comm *comm_ptr);
int mm_release_vc_table(MM_VCTABLE *p);

/*
What is an xfer block? - A block is defined by an init call, followed by one or more
xfer operations, and finally a start call.
Priority in a block is defined by the order of operations issued in a block.
Lower priority operations may make progress as long as they can be preempted
by higher priority operations.

Progress on blocks may occur in parallel.  Blocks are independent.

dependencies:
1) completion
2) progress
3) resource (buffer)

Xfer blocks are used to create graphs of cars. Multiple cars within an xfer block 
may be aggregated in the order issued.

mop = MPI operation
Do not add xfer_gather/scatter_mop_send_op because it is not necessary at the 
xfer level.

The xfer...mop functions generate cars like this:
(recv_mop) - (send)
(recv) - (mop_send)

The mop_send exists at the car level, not the xfer level.

xfer_gather/scatter_recv_forward_op generates these cars:
(recv) - (send)

recv_mop can be used for accumulate

send_mop could cause remote operations to occur.  We will not use send_mop 
currently.
*/
int xfer_gather_init(int dest, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr);
int xfer_gather_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_recv_mop_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_recv_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int src);
int xfer_gather_forward_op(MPID_Request *request_ptr, int size);
int xfer_gather_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last);
int xfer_gather_start(MPID_Request *request_ptr);

int xfer_scatter_init(int src, int tag, MPID_Comm *comm_ptr, MPID_Request **request_pptr);
int xfer_scatter_recv_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last);
int xfer_scatter_recv_mop_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last);
int xfer_scatter_recv_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_scatter_recv_mop_forward_op(MPID_Request *request_ptr, void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_scatter_forward_op(MPID_Request *request_ptr, int size, int dest);
int xfer_scatter_send_op(MPID_Request *request_ptr, const void *buf, int count, MPI_Datatype dtype, int first, int last, int dest);
int xfer_scatter_replicate_op(MPID_Request *request_ptr, int dest);
int xfer_scatter_start(MPID_Request *request_ptr);

#endif
