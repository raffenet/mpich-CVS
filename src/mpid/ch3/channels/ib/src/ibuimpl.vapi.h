/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IBUIMPL_VAPI_H
#define IBUIMPL_VAPI_H

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define IBU_ERROR_MSG_LENGTH       255
#define IBU_PACKET_SIZE            (1024 * 64)
#define IBU_PACKET_COUNT           128
#define IBU_NUM_PREPOSTED_RECEIVES (IBU_ACK_WATER_LEVEL*3)
#define IBU_MAX_POSTED_SENDS       8192
#define IBU_MAX_DATA_SEGMENTS      100
#define IBU_ACK_WATER_LEVEL        32

#ifdef MPICH_DBG_OUTPUT
extern int g_num_received;
extern int g_num_sent;
extern int g_num_posted_receives;
extern int g_num_posted_sends;
extern int g_ps;
extern int g_pr;
#endif

typedef struct ibuBlock_t
{
    struct ibuBlock_t *next;
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    unsigned char data[IBU_PACKET_SIZE];
} ibuBlock_t;

typedef struct ibuQueue_t
{
    struct ibuQueue_t *next_q;
    ibuBlock_t *pNextFree;
    ibuBlock_t block[IBU_PACKET_COUNT];
} ibuQueue_t;

extern int g_offset;
#define GETLKEY(p) (((ibuBlock_t*)((char *)p - g_offset))->lkey)

struct ibuBlockAllocator_struct
{
    void **pNextFree;
    void *(* alloc_fn)(size_t size);
    void (* free_fn)(void *p);
    struct ibuBlockAllocator_struct *pNextAllocation;
    unsigned int nBlockSize;
    int nCount, nIncrementSize;
};

typedef struct ibuBlockAllocator_struct * ibuBlockAllocator;

#ifdef HAVE_32BIT_POINTERS

typedef union ibu_work_id_handle_t
{
    u_int64_t id;
    struct ibu_data
    {
	u_int32_t ptr, mem;
    } data;
} ibu_work_id_handle_t;

#else

typedef struct ibu_work_id_handle_t
{
    void *ptr, *mem;
} ibu_work_id_handle_t;

extern ibuBlockAllocator g_workAllocator /*= NULL*/;

#endif

typedef int IBU_STATE;
#define IBU_READING      0x0001
#define IBU_WRITING      0x0002
#define IBU_RDMA_WRITING 0x0004
#define IBU_RDMA_READING 0x0008

typedef struct ibu_buffer_t
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int iovlen;
    int index;
    int total;
} ibu_buffer_t;

typedef struct ibu_unex_read_t
{
    void *mem_ptr, *mem_ptr_orig;
    unsigned char *buf;
    unsigned int length;
    struct ibu_unex_read_t *next;
} ibu_unex_read_t;

typedef struct ibu_num_written_node_t
{
    int num_bytes;
    struct ibu_num_written_node_t *next;
} ibu_num_written_node_t;

typedef struct ibu_state_t
{
    IBU_STATE state;
    VAPI_qp_hndl_t qp_handle;
    /*ibuBlockAllocator allocator;*/
    ibuQueue_t *allocator;

    IB_lid_t dlid;
    VAPI_qp_num_t qp_num, dest_qp_num;

    int closing;
    int pending_operations;
    /* read and write structures */
    ibu_buffer_t read;
    ibu_unex_read_t *unex_list;
    ibu_buffer_t write;
    int nAvailRemote, nUnacked;
    /* vc pointer */
    MPIDI_VC_t *vc_ptr;
    /*void *user_ptr;*/
    /* unexpected queue pointer */
    struct ibu_state_t *unex_finished_queue;
    int max_inline_size;
} ibu_state_t;

typedef struct IBU_Global {
    VAPI_hca_hndl_t  hca_handle;
    VAPI_pd_hndl_t   pd_handle;
    VAPI_hca_port_t  hca_port;
    int              port;
    VAPI_cqe_num_t   cq_size;
    IB_lid_t         lid;
    ibu_state_t *    unex_finished_list;
    int              error;
    char             err_msg[IBU_ERROR_MSG_LENGTH];
    /* hack to get around zero sized messages */
    void *           ack_mem_ptr;
    /*VAPI_mr_hndl_t   ack_mr_handle;*/
    VAPI_lkey_t      ack_lkey;
} IBU_Global;

extern IBU_Global IBU_Process;

typedef struct ibu_num_written_t
{
    void *mem_ptr;
    int length;
} ibu_num_written_t;

extern ibu_num_written_t g_num_bytes_written_stack[IBU_MAX_POSTED_SENDS];
extern int g_cur_write_stack_index /*= 0*/;

/* prototypes */
int ibui_post_receive(ibu_t ibu);
int ibui_post_receive_unacked(ibu_t ibu);
#if 0
int ibui_post_write(ibu_t ibu, void *buf, int len);
int ibui_post_writev(ibu_t ibu, MPID_IOV *iov, int n);
#endif
int ibui_post_ack_write(ibu_t ibu);
int ibui_buffer_unex_read(ibu_t ibu, void *mem_ptr, unsigned int offset, unsigned int num_bytes);

/* utility allocator functions */

ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(size_t size), void (* free_fn)(void *p));
ibuQueue_t * ibuBlockAllocInitIB();
int ibuBlockAllocFinalize(ibuBlockAllocator *p);
int ibuBlockAllocFinalizeIB(ibuQueue_t *p);
void * ibuBlockAlloc(ibuBlockAllocator p);
void * ibuBlockAllocIB(ibuQueue_t *p);
int ibuBlockFree(ibuBlockAllocator p, void *pBlock);
int ibuBlockFreeIB(ibuQueue_t *p, void *pBlock);
void *ib_malloc_register(size_t size, VAPI_mr_hndl_t *mhp, VAPI_lkey_t *lp, VAPI_rkey_t *rp);
void ib_free_deregister(void *p);

#endif
