/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "ibu.h"
#include "iba.h"
#include "psc_iba.h"
#include <stdio.h>

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

typedef union ibu_work_id_handle_t
{
    ib_uint64_t id;
    struct ibu_data
    {
	ib_uint32_t ptr, mem;
    } data;
} ibu_work_id_handle_t;

typedef int IBU_STATE;
#define IBU_ACCEPTING  0x0001
#define IBU_ACCEPTED   0x0002
#define IBU_CONNECTING 0x0004
#define IBU_READING    0x0008
#define IBU_WRITING    0x0010

typedef struct ibu_buffer
{
    int use_iov;
    unsigned int num_bytes;
    void *buffer;
    unsigned int bufflen;
    IBU_IOV iov[IBU_IOV_MAXLEN];
    int iovlen;
    int index;
    int total;
    int (*progress_update)(int,void*);
} ibu_buffer;

typedef struct ibu_unex_read_t
{
    void *mem_ptr;
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
    ib_uint32_t lkey;
    ib_qp_handle_t qp_handle;
    ibuBlockAllocator allocator;

    ib_uint32_t mtu_size;
    ib_uint32_t dlid;
    ib_mr_handle_t mr_handle;
    ib_uint32_t dest_qp_num;

    int closing;
    int pending_operations;
    /* read and write structures */
    ibu_buffer read;
    ibu_unex_read_t *unex_list;
    ibu_buffer write;
    int nAvailRemote, nUnacked;
    /* user pointer */
    void *user_ptr;
    /* unexpected queue pointer */
    struct ibu_state_t *unex_finished_queue;
} ibu_state_t;

#define IBU_ERROR_MSG_LENGTH       255
#define IBU_PACKET_SIZE            (1024 * 64)
#define IBU_PACKET_COUNT           64
#define IBU_NUM_PREPOSTED_RECEIVES (IBU_ACK_WATER_LEVEL*2)
#define IBU_MAX_CQ_ENTRIES         255
#define IBU_MAX_POSTED_SENDS       8192
#define IBU_MAX_DATA_SEGMENTS      100
#define IBU_ACK_WATER_LEVEL        16

typedef struct IBU_Global {
       ib_hca_handle_t hca_handle;
        ib_pd_handle_t pd_handle;
       ib_cqd_handle_t cqd_handle;
                   int lid;
       ib_hca_attr_t * attr_p;
         ibu_state_t * unex_finished_list;
		   int error;
		  char err_msg[IBU_ERROR_MSG_LENGTH];
} IBU_Global;

IBU_Global IBU_Process;

#define DEFAULT_NUM_RETRIES 10

static int g_connection_attempts = DEFAULT_NUM_RETRIES;
static int g_num_cp_threads = 2;

typedef struct ibu_num_written_t
{
    void *mem_ptr;
    int length;
} ibu_num_written_t;

static ibu_num_written_t g_num_bytes_written_stack[IBU_MAX_POSTED_SENDS];
static int g_cur_write_stack_index = 0;

/* local prototypes */
static int ibui_post_receive(ibu_t ibu);
static int ibui_post_receive_unacked(ibu_t ibu);
static int ibui_post_write(ibu_t ibu, void *buf, int len, int (*write_progress_update)(int, void*));
static int ibui_post_writev(ibu_t ibu, IBU_IOV *iov, int n, int (*write_progress_update)(int, void*));
static int ibui_post_ack_write(ibu_t ibu);

/* utility allocator functions */

static ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p));
static int ibuBlockAllocFinalize(ibuBlockAllocator *p);
static void * ibuBlockAlloc(ibuBlockAllocator p);
static int ibuBlockFree(ibuBlockAllocator p, void *pBlock);

static ibuBlockAllocator ibuBlockAllocInit(unsigned int blocksize, int count, int incrementsize, void *(* alloc_fn)(unsigned int size), void (* free_fn)(void *p))
{
    ibuBlockAllocator p;
    void **ppVoid;
    int i;

    p = alloc_fn( sizeof(struct ibuBlockAllocator_struct) + ((blocksize + sizeof(void**)) * count) );

    p->alloc_fn = alloc_fn;
    p->free_fn = free_fn;
    p->nIncrementSize = incrementsize;
    p->pNextAllocation = NULL;
    p->nCount = count;
    p->nBlockSize = blocksize;
    p->pNextFree = (void**)(p + 1);

    ppVoid = (void**)(p + 1);
    for (i=0; i<count-1; i++)
    {
	*ppVoid = (void*)((char*)ppVoid + sizeof(void**) + blocksize);
	ppVoid = *ppVoid;
    }
    *ppVoid = NULL;

    return p;
}

static int ibuBlockAllocFinalize(ibuBlockAllocator *p)
{
    if (*p == NULL)
	return 0;
    ibuBlockAllocFinalize(&(*p)->pNextAllocation);
    if ((*p)->free_fn != NULL)
	(*p)->free_fn(*p);
    *p = NULL;
    return 0;
}

static void * ibuBlockAlloc(ibuBlockAllocator p)
{
    void *pVoid;
    
    if (p->pNextFree == NULL)
    {
	MPIU_DBG_PRINTF(("ibuBlockAlloc returning NULL\n"));
	return NULL;
    }

    pVoid = p->pNextFree + 1;
    p->pNextFree = *(p->pNextFree);

    return pVoid;
}

static int ibuBlockFree(ibuBlockAllocator p, void *pBlock)
{
    ((void**)pBlock)--;
    *((void**)pBlock) = p->pNextFree;
    p->pNextFree = pBlock;

    return 0;
}

/* utility ibu functions */

#undef FUNCNAME
#define FUNCNAME modifyQP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static ib_uint32_t modifyQP( ibu_t ibu, Ib_qp_state qp_state )
{
    ib_uint32_t status;
    ib_qp_attr_list_t attrList;
    ib_address_vector_t av;
    attr_rec_t *attr_rec = NULL;
    MPIDI_STATE_DECL(MPID_STATE_IBU_MODIFYQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_MODIFYQP);

    if (qp_state == IB_QP_STATE_INIT)
    {
	if ((attr_rec = (attr_rec_t *)
	     MPIU_Malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    err_printf("%s: Malloc failed %d\n", FCNAME, __LINE__);
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	    return IBU_FAIL;
	}
	    
	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_PORT;
	((attr_rec[0]).data) = 1;
	((attr_rec[1]).id) = IB_QP_ATTR_PRIMARY_P_KEY_IX;
	((attr_rec[1]).data) = 0;
	((attr_rec[2]).id) = IB_QP_ATTR_RDMA_W_F;
	((attr_rec[2]).data) = 1;
	((attr_rec[3]).id) = IB_QP_ATTR_RDMA_R_F;
	((attr_rec[3]).data) = 1;
	((attr_rec[4]).id) = IB_QP_ATTR_ATOMIC_F;
	((attr_rec[4]).data) = 0;
	    
	attrList.attr_num = 5;
	attrList.attr_rec_p = &attr_rec[0];    
    }
    else if (qp_state == IB_QP_STATE_RTR) 
    {
	av.sl                         = 0;
	/*MPIU_DBG_PRINTF(("setting dest_lid to ibu->dlid: %d\n", ibu->dlid));*/
	av.dest_lid                   = (ib_uint16_t)ibu->dlid;
	av.grh_f                      = 0;
	av.path_bits                  = 0;
	av.max_static_rate            = 1;
	av.global.flow_label          = 1;
	av.global.hop_limit           = 1;
	av.global.src_gid_index       = 0;
	av.global.traffic_class       = 1;
	    
	if ((attr_rec = (attr_rec_t *)
	     MPIU_Malloc(sizeof (attr_rec_t) * 6)) == NULL )
	{
	    err_printf("%s: Malloc failed %d\n", FCNAME, __LINE__);
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	    return IBU_FAIL;
	}

	((attr_rec[0]).id) = IB_QP_ATTR_PRIMARY_ADDR;
	((attr_rec[0]).data) = (int)&av;
	((attr_rec[1]).id) = IB_QP_ATTR_DEST_QPN;
	((attr_rec[1]).data) = ibu->dest_qp_num;
	((attr_rec[2]).id) = IB_QP_ATTR_RCV_PSN;
	((attr_rec[2]).data) = 0;
	((attr_rec[3]).id) = IB_QP_ATTR_MTU;
	((attr_rec[3]).data) = ibu->mtu_size;
	((attr_rec[4]).id) = IB_QP_ATTR_RDMA_READ_LIMIT;
	((attr_rec[4]).data) = 4;
	((attr_rec[5]).id) = IB_QP_ATTR_RNR_NAK_TIMER;
	((attr_rec[5]).data) = 1;
	    
	attrList.attr_num = 6;
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RTS)
    {
	if ((attr_rec = (attr_rec_t *)
	     MPIU_Malloc(sizeof (attr_rec_t) * 5)) == NULL )
	{
	    err_printf("%s: Malloc failed %d\n", FCNAME, __LINE__);
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	    return IBU_FAIL;
	}

	((attr_rec[0]).id)    = IB_QP_ATTR_SEND_PSN;
	((attr_rec[0]).data)  = 0; 
	((attr_rec[1]).id)    = IB_QP_ATTR_TIMEOUT;
	((attr_rec[1]).data)  = 0x7c;
	((attr_rec[2]).id)    = IB_QP_ATTR_RETRY_COUNT;
	((attr_rec[2]).data)  = 2048;
	((attr_rec[3]).id)    = IB_QP_ATTR_RNR_RETRY_COUNT;
	((attr_rec[3]).data)  = 2048;
	((attr_rec[4]).id)    = IB_QP_ATTR_DEST_RDMA_READ_LIMIT;
	((attr_rec[4]).data)  = 4;
	
	attrList.attr_num = 5; 
	attrList.attr_rec_p = &attr_rec[0];
    }
    else if (qp_state == IB_QP_STATE_RESET)
    {
	attrList.attr_num = 0;
	attrList.attr_rec_p = NULL;
    }
    else
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return IBU_FAIL;
    }

    status = ib_qp_modify_us(IBU_Process.hca_handle, 
			     ibu->qp_handle, 
			     qp_state, 
			     &attrList );
    if (attr_rec)    
	MPIU_Free(attr_rec);
    if( status != IBU_SUCCESS )
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
	return status;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_MODIFYQP);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME createQP
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static ib_uint32_t createQP(ibu_t ibu, ibu_set_t set)
{
    ib_uint32_t status;
    ib_qp_attr_list_t attrList;
    attr_rec_t attr_rec[] = {
	{IB_QP_ATTR_SERVICE_TYPE, IB_ST_RELIABLE_CONNECTION},
	{IB_QP_ATTR_SEND_CQ, 0},
	{IB_QP_ATTR_RCV_CQ, 0},
	{IB_QP_ATTR_SEND_REQ_MAX, 0},
	{IB_QP_ATTR_RCV_REQ_MAX, 0},
	{IB_QP_ATTR_SEND_SGE_MAX, 8},
	{IB_QP_ATTR_RCV_SGE_MAX, 8},
	{IB_QP_ATTR_SIGNALING_TYPE, QP_SIGNAL_ALL}
    };
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATEQP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATEQP);

    attr_rec[1].data = (int)set;
    attr_rec[2].data = (int)set;
    attr_rec[3].data = 255;
    attr_rec[4].data = 255;

    attrList.attr_num = sizeof(attr_rec)/sizeof(attr_rec[0]);
    attrList.attr_rec_p = &attr_rec[0];

    status = ib_qp_create_us(
	IBU_Process.hca_handle,
	IBU_Process.pd_handle,
	&attrList, 
	&ibu->qp_handle, 
	&ibu->dest_qp_num, 
	NULL);
    if (status != IBA_OK)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
	return status;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATEQP);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ib_malloc_register
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static ib_mr_handle_t s_mr_handle;
static ib_uint32_t    s_lkey;
static void *ib_malloc_register(size_t size)
{
    ib_uint32_t status;
    void *ptr;
    ib_uint32_t rkey;
    MPIDI_STATE_DECL(MPID_STATE_IB_MALLOC_REGISTER);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_MALLOC_REGISTER);

    ptr = MPIU_Malloc(size);
    if (ptr == NULL)
    {
	err_printf("ib_malloc_register: MPIU_Malloc(%d) failed.\n", size);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }
    status = ib_mr_register_us(
	IBU_Process.hca_handle,
	(ib_uint8_t*)ptr,
	size,
	IBU_Process.pd_handle,
	IB_ACCESS_LOCAL_WRITE,
	&s_mr_handle,
	&s_lkey, &rkey);
    if (status != IBU_SUCCESS)
    {
	err_printf("ib_malloc_register: ib_mr_register_us failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
	return NULL;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IB_MALLOC_REGISTER);
    return ptr;
}

#undef FUNCNAME
#define FUNCNAME ib_free_deregister
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static void ib_free_deregister(void *p)
{
    MPIDI_STATE_DECL(MPID_STATE_IB_FREE_DEREGISTER);

    MPIDI_FUNC_ENTER(MPID_STATE_IB_FREE_DEREGISTER);
    /*ib_mr_deregister_us(IBU_Process.hca_handle, s_mr_handle);*/
    MPIU_Free(p);
    MPIDI_FUNC_EXIT(MPID_STATE_IB_FREE_DEREGISTER);
}

#undef FUNCNAME
#define FUNCNAME ibu_create_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
ibu_t ibu_create_qp(ibu_set_t set, int dlid)
{
    ib_uint32_t status;
    ibu_t p;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_QP);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_QP);

    p = (ibu_t)MPIU_Malloc(sizeof(ibu_state_t));
    if (p == NULL)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_QP);
	return NULL;
    }

    p->dlid = dlid;
    /* In ibuBlockAllocInit, ib_malloc_register is called which sets the global variable s_mr_handle */
    p->allocator = ibuBlockAllocInit(IBU_PACKET_SIZE, IBU_PACKET_COUNT, IBU_PACKET_COUNT, ib_malloc_register, ib_free_deregister);
    p->mr_handle = s_mr_handle; /* Not thread safe. This handle is reset every time ib_malloc_register is called. */
    p->mtu_size = 3; /* 3 = 2048 */
    /* save the lkey for posting sends and receives */
    p->lkey = s_lkey;

    /*MPIDI_DBG_PRINTF((60, FCNAME, "creating the queue pair\n"));*/
    /* Create the queue pair */
    status = createQP(p, set);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_create_qp: createQP failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_QP);
	return NULL;
    }

    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(INIT)"));*/
    status = modifyQP(p, IB_QP_STATE_INIT);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_create_qp: modifyQP(INIT) failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_QP);
	return NULL;
    }
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTR)"));*/
    status = modifyQP(p, IB_QP_STATE_RTR);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_create_qp: modifyQP(RTR) failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_QP);
	return NULL;
    }
    /*MPIDI_DBG_PRINTF((60, FCNAME, "modifyQP(RTS)"));*/
    status = modifyQP(p, IB_QP_STATE_RTS);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_create_qp: modifyQP(RTS) failed, error %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_QP);
	return NULL;
    }

    /* pre post some receives on each connection */
    p->nAvailRemote = 0;
    p->nUnacked = 0;
    for (i=0; i<IBU_NUM_PREPOSTED_RECEIVES; i++)
    {
	ibui_post_receive_unacked(p);
	p->nAvailRemote++; /* assumes the other side is executing this same code */
    }
    p->nAvailRemote--; /* remove one from nAvailRemote so a ack packet can always get through */
    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_QP);
    return p;
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#undef FUNCNAME
#define FUNCNAME ibui_post_receive_unacked
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_receive_unacked(ibu_t ibu)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_rcv_t work_req;
    void *mem_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RECEIVE);

    mem_ptr = ibuBlockAlloc(ibu->allocator);
    if (mem_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL"));
	return IBU_FAIL;
    }
    assert(mem_ptr);

    sg_list.data_seg_p = &data;
    sg_list.data_seg_num = 1;
    data.length = IBU_PACKET_SIZE;
    data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
    data.l_key = ibu->lkey;
    work_req.op_type = OP_RECEIVE;
    work_req.sg_list = sg_list;
    /* store the VC ptr and the mem ptr in the work id */
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;

    MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_rcv_req_us"));

    status = ib_post_rcv_req_us(IBU_Process.hca_handle, 
				ibu->qp_handle,
				&work_req);
    if (status != IBU_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	err_printf("%s: Error: failed to post ib receive, status = %d\n", FCNAME, status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return status;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_receive
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_receive(ibu_t ibu)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_rcv_t work_req;
    void *mem_ptr;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_RECEIVE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_RECEIVE);

    mem_ptr = ibuBlockAlloc(ibu->allocator);
    if (mem_ptr == NULL)
    {
	MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL"));
	return IBU_FAIL;
    }
    assert(mem_ptr);

    sg_list.data_seg_p = &data;
    sg_list.data_seg_num = 1;
    data.length = IBU_PACKET_SIZE;
    data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
    data.l_key = ibu->lkey;
    work_req.op_type = OP_RECEIVE;
    work_req.sg_list = sg_list;
    /* store the VC ptr and the mem ptr in the work id */
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;

    MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_rcv_req_us"));
    status = ib_post_rcv_req_us(IBU_Process.hca_handle, 
				ibu->qp_handle,
				&work_req);
    if (status != IBU_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	err_printf("%s: Error: failed to post ib receive, status = %d\n", FCNAME, status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
	return status;
    }
    if (++ibu->nUnacked > IBU_ACK_WATER_LEVEL)
    {
	ibui_post_ack_write(ibu);
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_RECEIVE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_ack_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_post_ack_write(ibu_t ibu)
{
    ib_uint32_t status;
    ib_work_req_send_t work_req;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_POST_ACK_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_POST_ACK_WRITE);

    work_req.dest_address      = 0;
    work_req.dest_q_key        = 0;
    work_req.dest_qpn          = 0;
    work_req.eecn              = 0;
    work_req.ethertype         = 0;
    work_req.fence_f           = 0;
    work_req.immediate_data    = ibu->nUnacked;
    work_req.immediate_data_f  = 1;
    work_req.op_type           = OP_SEND;
    work_req.remote_addr.va    = 0;
    work_req.remote_addr.key   = 0;
    work_req.se_f              = 0;
    work_req.sg_list.data_seg_num = 0;
    work_req.sg_list.data_seg_p = NULL;
    work_req.signaled_f        = 0;

    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
    ((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)-1;
    
    MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_send_req_us(%d byte ack)", ibu->nUnacked));
    status = ib_post_send_req_us( IBU_Process.hca_handle,
	ibu->qp_handle, 
	&work_req);
    if (status != IBU_SUCCESS)
    {
	MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	err_printf("%s: Error: failed to post ib send, status = %d, %s\n", FCNAME, status, iba_errstr(status));
	MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
	return status;
    }
    ibu->nUnacked = 0;

    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_POST_ACK_WRITE);
    return IBU_SUCCESS;
}

/* ibu functions */

#undef FUNCNAME
#define FUNCNAME ibui_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_write(ibu_t ibu, void *buf, int len)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_send_t work_req;
    void *mem_ptr;
    int length;
    int total = 0;
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITE);

    while (len)
    {
	length = min(len, IBU_PACKET_SIZE);
	len -= length;

	if (ibu->nAvailRemote < 1)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "no more remote packets available"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return total;
	}

	mem_ptr = ibuBlockAlloc(ibu->allocator);
	if (mem_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return total;
	}
	memcpy(mem_ptr, buf, length);
	total += length;
	
	MPIDI_DBG_PRINTF((60, FCNAME, "g_write_stack[%d].length = %d\n", g_cur_write_stack_index, length));
	g_num_bytes_written_stack[g_cur_write_stack_index].length = length;
	g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr = mem_ptr;
	g_cur_write_stack_index++;

	sg_list.data_seg_p = &data;
	sg_list.data_seg_num = 1;
	data.length = length;
	data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
	data.l_key = ibu->lkey;
	
	work_req.dest_address      = 0;
	work_req.dest_q_key        = 0;
	work_req.dest_qpn          = 0;
	work_req.eecn              = 0;
	work_req.ethertype         = 0;
	work_req.fence_f           = 0;
	work_req.immediate_data    = 0;
	work_req.immediate_data_f  = 0;
	work_req.op_type           = OP_SEND;
	work_req.remote_addr.va    = 0;
	work_req.remote_addr.key   = 0;
	work_req.se_f              = 0;
	work_req.sg_list           = sg_list;
	work_req.signaled_f        = 0;
	
	/* store the ibu ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;
	
	MPIDI_DBG_PRINTF((60, FCNAME, "calling ib_post_send_req_us(%d bytes)", length));
	status = ib_post_send_req_us( IBU_Process.hca_handle,
	    ibu->qp_handle, 
	    &work_req);
	if (status != IBU_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	    err_printf("%s: Error: failed to post ib send, status = %d, %s\n", FCNAME, status, iba_errstr(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
	    return -1;
	}
	ibu->nAvailRemote--;

	buf = (char*)buf + length;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITE);
    return total;
}

#undef FUNCNAME
#define FUNCNAME ibui_post_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_writev(ibu_t ibu, IBU_IOV *iov, int n)
{
    ib_uint32_t status;
    ib_scatter_gather_list_t sg_list;
    ib_data_segment_t data;
    ib_work_req_send_t work_req;
    void *mem_ptr;
    unsigned int len, msg_size;
    int total = 0;
    unsigned int num_avail;
    unsigned char *buf;
    int cur_index;
    unsigned int cur_len;
    unsigned char *cur_buf;
    MPIDI_STATE_DECL(MPID_STATE_IBU_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WRITEV);

    cur_index = 0;
    cur_len = iov[0].IBU_IOV_LEN;
    cur_buf = iov[0].IBU_IOV_BUF;
    do
    {
	if (ibu->nAvailRemote < 1)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "no more remote packets available."));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return total;
	}
	mem_ptr = ibuBlockAlloc(ibu->allocator);
	if (mem_ptr == NULL)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockAlloc returned NULL."));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return total;
	}
	buf = mem_ptr;
	num_avail = IBU_PACKET_SIZE;
	/*MPIU_DBG_PRINTF(("iov length: %d\n", n));*/
	for (; cur_index < n && num_avail; )
	{
	    len = min (num_avail, cur_len);
	    num_avail -= len;
	    total += len;
	    /*MPIU_DBG_PRINTF(("copying %d bytes to ib buffer - num_avail: %d\n", len, num_avail));*/
	    memcpy(buf, cur_buf, len);
	    buf += len;
	    
	    if (cur_len == len)
	    {
		cur_index++;
		cur_len = iov[cur_index].IBU_IOV_LEN;
		cur_buf = iov[cur_index].IBU_IOV_BUF;
	    }
	    else
	    {
		cur_len -= len;
		cur_buf += len;
	    }
	}
	msg_size = IBU_PACKET_SIZE - num_avail;
	
	MPIDI_DBG_PRINTF((60, FCNAME, "g_write_stack[%d].length = %d\n", g_cur_write_stack_index, msg_size));
	g_num_bytes_written_stack[g_cur_write_stack_index].length = msg_size;
	g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr = mem_ptr;
	g_cur_write_stack_index++;
	
	data.length = msg_size;
	data.va = (ib_uint64_t)(ib_uint32_t)mem_ptr;
	data.l_key = ibu->lkey;
	
	sg_list.data_seg_p = &data;
	sg_list.data_seg_num = 1;
	
	work_req.dest_address      = 0;
	work_req.dest_q_key        = 0;
	work_req.dest_qpn          = 0;
	work_req.eecn              = 0;
	work_req.ethertype         = 0;
	work_req.fence_f           = 0;
	work_req.immediate_data    = 0;
	work_req.immediate_data_f  = 0;
	work_req.op_type           = OP_SEND;
	work_req.remote_addr.va    = 0;
	work_req.remote_addr.key   = 0;
	work_req.se_f              = 0;
	work_req.sg_list           = sg_list;
	work_req.signaled_f        = 0;
	
	/* store the ibu ptr and the mem ptr in the work id */
	((ibu_work_id_handle_t*)&work_req.work_req_id)->data.ptr = (ib_uint32_t)ibu;
	((ibu_work_id_handle_t*)&work_req.work_req_id)->data.mem = (ib_uint32_t)mem_ptr;
	
	MPIDI_DBG_PRINTF((60, FCNAME, "ib_post_send_req_us(%d bytes)", msg_size));
	status = ib_post_send_req_us( IBU_Process.hca_handle,
	    ibu->qp_handle, 
	    &work_req);
	if (status != IBU_SUCCESS)
	{
	    MPIU_DBG_PRINTF(("%s: nAvailRemote: %d, nUnacked: %d\n", FCNAME, ibu->nAvailRemote, ibu->nUnacked));
	    err_printf("%s: Error: failed to post ib send, status = %d, %s\n", FCNAME, status, iba_errstr(status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
	    return -1;
	}
	ibu->nAvailRemote--;
	
    } while (cur_index < n);
    
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WRITEV);
    return total;
}

#undef FUNCNAME
#define FUNCNAME ibu_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_init()
{
    ib_uint32_t status;
    ib_uint32_t max_cq_entries = IBU_MAX_CQ_ENTRIES+1;
    ib_uint32_t attr_size;
    MPIDI_STATE_DECL(MPID_STATE_IBU_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_INIT);

    /*ib_init_us();*/ /* for some reason Paceline does not support the init function */

    /* Initialize globals */
    /* get a handle to the host channel adapter */
    status = ib_hca_open_us(0 , &IBU_Process.hca_handle);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_init: ib_hca_open_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    /* get a protection domain handle */
    status = ib_pd_allocate_us(IBU_Process.hca_handle, &IBU_Process.pd_handle);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_init: ib_pd_allocate_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    /* get a completion queue domain handle */
    status = ib_cqd_create_us(IBU_Process.hca_handle, &IBU_Process.cqd_handle);
#if 0 /* for some reason this function fails when it really is ok */
    if (status != IBU_SUCCESS)
    {
	err_printf("ib_init: ib_cqd_create_us failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
#endif

    /* get the lid */
    attr_size = 0;
    status = ib_hca_query_us(IBU_Process.hca_handle, NULL, 
        HCA_QUERY_HCA_STATIC | HCA_QUERY_PORT_INFO_STATIC | HCA_QUERY_PORT_INFO_DYNAMIC, &attr_size);
    IBU_Process.attr_p = MPIU_Calloc(attr_size, sizeof(ib_uint8_t));
    status = ib_hca_query_us(IBU_Process.hca_handle, IBU_Process.attr_p, 
	HCA_QUERY_HCA_STATIC | HCA_QUERY_PORT_INFO_STATIC | HCA_QUERY_PORT_INFO_DYNAMIC, &attr_size);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_init: ib_hca_query_us(HCA_QUERY_HCA_STATIC) failed, status %d\n", status);
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
	return status;
    }
    IBU_Process.lid = IBU_Process.attr_p->port_dynamic_info_p->lid;
    /*
    MPIU_DBG_PRINTF(("infiniband:\n mtu: %d\n msg_size: %d\n",
	IBU_Process.attr_p->port_static_info_p->mtu,
	IBU_Process.attr_p->port_static_info_p->msg_size));
    */

    /* non infiniband initialization */
    IBU_Process.unex_finished_list = NULL;

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_INIT);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_finalize()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_FINALIZE);
    /*ib_release_us();*/ /* for some reason Paceline does not support the release function */
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_FINALIZE);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_create_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_create_set(ibu_set_t *set)
{
    ib_uint32_t status;
    ib_uint32_t max_cq_entries = IBU_MAX_CQ_ENTRIES+1;
    MPIDI_STATE_DECL(MPID_STATE_IBU_CREATE_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_CREATE_SET);
    /* create the completion queue */
    status = ib_cq_create_us(
	IBU_Process.hca_handle, 
	IBU_Process.cqd_handle,
	&max_cq_entries,
	set,
	NULL);
    if (status != IBU_SUCCESS)
    {
	err_printf("ibu_init: ib_cq_create_us failed, error %d\n", status);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_CREATE_SET);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibu_destroy_set
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_destroy_set(ibu_set_t set)
{
    ib_uint32_t status;
    MPIDI_STATE_DECL(MPID_STATE_IBU_DESTROY_SET);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_DESTROY_SET);
    status = ib_cq_destroy_us(IBU_Process.hca_handle, set);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_DESTROY_SET);
    return status;
}

#undef FUNCNAME
#define FUNCNAME ibui_buffer_unex_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_buffer_unex_read(ibu_t ibu, void *mem_ptr, unsigned int offset, unsigned int num_bytes)
{
    ibu_unex_read_t *p;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_BUFFER_UNEX_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_BUFFER_UNEX_READ);

    MPIDI_DBG_PRINTF((60, FCNAME, "%d bytes\n", num_bytes));

    p = (ibu_unex_read_t *)MPIU_Malloc(sizeof(ibu_unex_read_t));
    p->mem_ptr = mem_ptr;
    p->buf = (unsigned char *)mem_ptr + offset;
    p->length = num_bytes;
    p->next = ibu->unex_list;
    ibu->unex_list = p;

    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_BUFFER_UNEX_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_read_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int ibui_read_unex(ibu_t ibu)
{
    unsigned int len;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_READ_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_READ_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    assert(ibu->unex_list);

    /* copy the received data */
    while (ibu->unex_list)
    {
	len = min(ibu->unex_list->length, ibu->read.bufflen);
	memcpy(ibu->read.buffer, ibu->unex_list->buf, len);
	/* advance the user pointer */
	ibu->read.buffer = (char*)(ibu->read.buffer) + len;
	ibu->read.bufflen -= len;
	ibu->read.total += len;
	if (len != ibu->unex_list->length)
	{
	    ibu->unex_list->length -= len;
	    ibu->unex_list->buf += len;
	}
	else
	{
	    /* put the receive packet back in the pool */
	    if (ibu->unex_list->mem_ptr == NULL)
	    {
		err_printf("ibui_read_unex: mem_ptr == NULL\n");
	    }
	    assert(ibu->unex_list->mem_ptr != NULL);
	    ibuBlockFree(ibu->allocator, ibu->unex_list->mem_ptr);
	    /* MPIU_Free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
	    /* post another receive to replace the consumed one */
	    ibui_post_receive(ibu);
	}
	/* check to see if the entire message was received */
	if (ibu->read.bufflen == 0)
	{
	    /* place this ibu in the finished list so it will be completed by ibu_wait */
	    ibu->state &= ~IBU_READING;
	    ibu->unex_finished_queue = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = ibu;
	    /* post another receive to replace the consumed one */
	    /*ibui_post_receive(ibu);*/
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in IBU_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READ_UNEX);
	    return IBU_SUCCESS;
	}
	/* make the user upcall */
	/*
	if (ibu->read.progress_update != NULL)
	ibu->read.progress_update(num_bytes, ibu->user_ptr);
	*/
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READ_UNEX);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibui_readv_unex
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibui_readv_unex(ibu_t ibu)
{
    unsigned int num_bytes;
    ibu_unex_read_t *temp;
    MPIDI_STATE_DECL(MPID_STATE_IBUI_READV_UNEX);

    MPIDI_FUNC_ENTER(MPID_STATE_IBUI_READV_UNEX);

    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));

    while (ibu->unex_list)
    {
	while (ibu->unex_list->length && ibu->read.iovlen)
	{
	    num_bytes = min(ibu->unex_list->length, ibu->read.iov[ibu->read.index].IBU_IOV_LEN);
	    MPIDI_DBG_PRINTF((60, FCNAME, "copying %d bytes\n", num_bytes));
	    /* copy the received data */
	    memcpy(ibu->read.iov[ibu->read.index].IBU_IOV_BUF, ibu->unex_list->buf, num_bytes);
	    ibu->read.total += num_bytes;
	    ibu->unex_list->buf += num_bytes;
	    ibu->unex_list->length -= num_bytes;
	    /* update the iov */
	    ibu->read.iov[ibu->read.index].IBU_IOV_LEN -= num_bytes;
	    ibu->read.iov[ibu->read.index].IBU_IOV_BUF = 
		(char*)(ibu->read.iov[ibu->read.index].IBU_IOV_BUF) + num_bytes;
	    if (ibu->read.iov[ibu->read.index].IBU_IOV_LEN == 0)
	    {
		ibu->read.index++;
		ibu->read.iovlen--;
	    }
	}

	if (ibu->unex_list->length == 0)
	{
	    /* put the receive packet back in the pool */
	    if (ibu->unex_list->mem_ptr == NULL)
	    {
		err_printf("ibui_readv_unex: mem_ptr == NULL\n");
	    }
	    assert(ibu->unex_list->mem_ptr != NULL);
	    MPIDI_DBG_PRINTF((60, FCNAME, "ibuBlockFree(mem_ptr)"));
	    ibuBlockFree(ibu->allocator, ibu->unex_list->mem_ptr);
	    /* MPIU_Free the unexpected data node */
	    temp = ibu->unex_list;
	    ibu->unex_list = ibu->unex_list->next;
	    MPIU_Free(temp);
	    /* replace the consumed read descriptor */
	    ibui_post_receive(ibu);
	}
	
	if (ibu->read.iovlen == 0)
	{
	    ibu->state &= ~IBU_READING;
	    ibu->unex_finished_queue = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = ibu;
	    MPIDI_DBG_PRINTF((60, FCNAME, "finished read saved in IBU_Process.unex_finished_list\n"));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READV_UNEX);
	    return IBU_SUCCESS;
	}
	/* make the user upcall */
	/*
	if (ibu->read.progress_update != NULL)
	ibu->read.progress_update(num_bytes, ibu->user_ptr);
	*/
    }
    MPIDI_FUNC_EXIT(MPID_STATE_IBUI_READV_UNEX);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_wait(ibu_set_t set, int millisecond_timeout, ibu_wait_t *out)
{
    int i;
    ib_uint32_t status;
    ib_work_completion_t completion_data;
    void *mem_ptr;
    char *iter_ptr;
    ibu_t ibu;
    int num_bytes;
    unsigned int offset;
    MPIDI_STATE_DECL(MPID_STATE_IBU_WAIT);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_WAIT);
    /*MPIDI_DBG_PRINTF((60, FCNAME, "entering"));*/
    for (;;) 
    {
	if (IBU_Process.unex_finished_list)
	{
	    MPIDI_DBG_PRINTF((60, FCNAME, "returning previously received %d bytes", IBU_Process.unex_finished_list->read.total));
	    /* remove this ibu from the finished list */
	    ibu = IBU_Process.unex_finished_list;
	    IBU_Process.unex_finished_list = IBU_Process.unex_finished_list->unex_finished_queue;
	    ibu->unex_finished_queue = NULL;

	    out->num_bytes = ibu->read.total;
	    out->op_type = IBU_OP_READ;
	    out->user_ptr = ibu->user_ptr;
	    ibu->pending_operations--;
	    if (ibu->closing && ibu->pending_operations == 0)
	    {
		ibu = IBU_INVALID_QP;
	    }
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_SUCCESS;
	}

	status = ib_completion_poll_us(
	    IBU_Process.hca_handle,
	    set,
	    &completion_data);
	if (status == IBA_CQ_EMPTY)
	{
	    /* ibu_wait polls until there is something in the queue */
	    /* or the timeout has expired */
	    if (millisecond_timeout == 0)
	    {
		out->num_bytes = 0;
		out->error = 0;
		out->user_ptr = NULL;
		out->op_type = IBU_OP_TIMEOUT;
		MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		return IBU_SUCCESS;
	    }
	    continue;
	}
	if (status != IBA_OK)
	{
	    err_printf("%s: error: ib_completion_poll_us did not return IBA_OK\n", FCNAME);
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}
	if (completion_data.status != IB_COMP_ST_SUCCESS)
	{
	    err_printf("%s: error: status = %d != IB_COMP_ST_SUCCESS, %s\n", 
		FCNAME, completion_data.status, iba_compstr(completion_data.status));
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_FAIL;
	}

	ibu = (ibu_t)(((ibu_work_id_handle_t*)&completion_data.work_req_id)->data.ptr);
	mem_ptr = (void*)(((ibu_work_id_handle_t*)&completion_data.work_req_id)->data.mem);

	switch (completion_data.op_type)
	{
	case OP_SEND:
	    if (completion_data.immediate_data_f || (int)mem_ptr == -1)
	    {
		/* flow control ack completed, no user data so break out here */
		break;
	    }
	    g_cur_write_stack_index--;
	    num_bytes = g_num_bytes_written_stack[g_cur_write_stack_index].length;
	    MPIDI_DBG_PRINTF((60, FCNAME, "send num_bytes = %d\n", num_bytes));
	    if (num_bytes < 0)
	    {
		i = num_bytes;
		num_bytes = 0;
		for (; i<0; i++)
		{
		    g_cur_write_stack_index--;
		    MPIDI_DBG_PRINTF((60, FCNAME, "num_bytes += %d\n", g_num_bytes_written_stack[g_cur_write_stack_index].length));
		    num_bytes += g_num_bytes_written_stack[g_cur_write_stack_index].length;
		    if (g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr == NULL)
			err_printf("ibu_wait: write stack has NULL mem_ptr at location %d\n", g_cur_write_stack_index);
		    assert(g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr != NULL);
		    ibuBlockFree(ibu->allocator, g_num_bytes_written_stack[g_cur_write_stack_index].mem_ptr);
		}
	    }
	    else
	    {
		if (mem_ptr == NULL)
		    err_printf("ibu_wait: send mem_ptr == NULL\n");
		assert(mem_ptr != NULL);
		ibuBlockFree(ibu->allocator, mem_ptr);
	    }

	    out->num_bytes = num_bytes;
	    out->op_type = IBU_OP_WRITE;
	    out->user_ptr = ibu->user_ptr;
	    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
	    return IBU_SUCCESS;
	    break;
	case OP_RECEIVE:
	    if (completion_data.immediate_data_f)
	    {
		ibu->nAvailRemote += completion_data.immediate_data;
		MPIDI_DBG_PRINTF((60, FCNAME, "%d packets acked, nAvailRemote now = %d", completion_data.immediate_data, ibu->nAvailRemote));
		ibuBlockFree(ibu->allocator, mem_ptr);
		ibui_post_receive_unacked(ibu);
		assert(completion_data.bytes_num == 0); /* check this after the printfs to see if the immediate data is correct */
		break;
	    }
	    num_bytes = completion_data.bytes_num;
	    MPIDI_DBG_PRINTF((60, FCNAME, "read %d bytes\n", num_bytes));
	    /*MPIDI_DBG_PRINTF((60, FCNAME, "ibu_wait(recv finished %d bytes)", num_bytes));*/
	    if (!(ibu->state & IBU_READING))
	    {
		ibui_buffer_unex_read(ibu, mem_ptr, 0, num_bytes);
		break;
	    }
	    MPIDI_DBG_PRINTF((60, FCNAME, "read update, total = %d + %d = %d\n", ibu->read.total, num_bytes, ibu->read.total + num_bytes));
	    if (ibu->read.use_iov)
	    {
		iter_ptr = mem_ptr;
		while (num_bytes && ibu->read.iovlen > 0)
		{
		    if ((int)ibu->read.iov[ibu->read.index].IBU_IOV_LEN <= num_bytes)
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].IBU_IOV_BUF, iter_ptr,
			    ibu->read.iov[ibu->read.index].IBU_IOV_LEN);
			iter_ptr += ibu->read.iov[ibu->read.index].IBU_IOV_LEN;
			/* update the iov */
			num_bytes -= ibu->read.iov[ibu->read.index].IBU_IOV_LEN;
			ibu->read.index++;
			ibu->read.iovlen--;
		    }
		    else
		    {
			/* copy the received data */
			memcpy(ibu->read.iov[ibu->read.index].IBU_IOV_BUF, iter_ptr, num_bytes);
			iter_ptr += num_bytes;
			/* update the iov */
			ibu->read.iov[ibu->read.index].IBU_IOV_LEN -= num_bytes;
			ibu->read.iov[ibu->read.index].IBU_IOV_BUF = 
			    (char*)(ibu->read.iov[ibu->read.index].IBU_IOV_BUF) + num_bytes;
			num_bytes = 0;
		    }
		}
		offset = (unsigned char*)iter_ptr - (unsigned char*)mem_ptr;
		ibu->read.total += offset;
		if (num_bytes == 0)
		{
		    /* put the receive packet back in the pool */
		    if (mem_ptr == NULL)
			err_printf("ibu_wait: read mem_ptr == NULL\n");
		    assert(mem_ptr != NULL);
		    ibuBlockFree(ibu->allocator, mem_ptr);
		    ibui_post_receive(ibu);
		}
		else
		{
		    /* save the unused but received data */
		    ibui_buffer_unex_read(ibu, mem_ptr, offset, num_bytes);
		}
		if (ibu->read.iovlen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    out->num_bytes = ibu->read.total;
		    out->op_type = IBU_OP_READ;
		    out->user_ptr = ibu->user_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibuet after iov read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return IBU_SUCCESS;
		}
		/* make the user upcall */
		if (ibu->read.progress_update != NULL)
		    ibu->read.progress_update(num_bytes, ibu->user_ptr);
	    }
	    else
	    {
		if ((unsigned int)num_bytes > ibu->read.bufflen)
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, ibu->read.bufflen);
		    ibu->read.total = ibu->read.bufflen;
		    ibui_buffer_unex_read(ibu, mem_ptr, ibu->read.bufflen, num_bytes - ibu->read.bufflen);
		    ibu->read.bufflen = 0;
		}
		else
		{
		    /* copy the received data */
		    memcpy(ibu->read.buffer, mem_ptr, num_bytes);
		    ibu->read.total += num_bytes;
		    /* advance the user pointer */
		    ibu->read.buffer = (char*)(ibu->read.buffer) + num_bytes;
		    ibu->read.bufflen -= num_bytes;
		    /* put the receive packet back in the pool */
		    ibuBlockFree(ibu->allocator, mem_ptr);
		    ibui_post_receive(ibu);
		}
		if (ibu->read.bufflen == 0)
		{
		    ibu->state &= ~IBU_READING;
		    out->num_bytes = ibu->read.total;
		    out->op_type = IBU_OP_READ;
		    out->user_ptr = ibu->user_ptr;
		    ibu->pending_operations--;
		    if (ibu->closing && ibu->pending_operations == 0)
		    {
			MPIDI_DBG_PRINTF((60, FCNAME, "closing ibu after simple read completed."));
			ibu = IBU_INVALID_QP;
		    }
		    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
		    return IBU_SUCCESS;
		}
		/* make the user upcall */
		if (ibu->read.progress_update != NULL)
		    ibu->read.progress_update(num_bytes, ibu->user_ptr);
	    }
	    break;
	default:
	    err_printf("%s: unknown ib op_type: %d\n", FCNAME, completion_data.op_type);
	    break;
	}
    }

    MPIDI_FUNC_EXIT(MPID_STATE_IBU_WAIT);
}

#undef FUNCNAME
#define FUNCNAME ibu_set_user_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_set_user_ptr(ibu_t ibu, void *user_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_SET_USER_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_SET_USER_PTR);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    if (ibu == IBU_INVALID_QP)
    {
	MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
	return IBU_FAIL;
    }
    ibu->user_ptr = user_ptr;
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_SET_USER_PTR);
    return IBU_SUCCESS;
}

/* non-blocking functions */

#undef FUNCNAME
#define FUNCNAME ibu_post_read
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_read(ibu_t ibu, void *buf, int len, int (*rfn)(int, void*))
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READ);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READ);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    ibu->read.total = 0;
    ibu->read.buffer = buf;
    ibu->read.bufflen = len;
    ibu->read.use_iov = FALSE;
    ibu->read.progress_update = rfn;
    ibu->state |= IBU_READING;
    ibu->pending_operations++;
    /* copy any pre-received data into the buffer */
    if (ibu->unex_list)
	ibui_read_unex(ibu);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READ);
    return IBU_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME ibu_post_readv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_readv(ibu_t ibu, IBU_IOV *iov, int n, int (*rfn)(int, void*))
{
#ifdef MPICH_DBG_OUTPUT
    char str[1024] = "ibu_post_readv: ";
    char *s;
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_READV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_READV);
#ifdef MPICH_DBG_OUTPUT
    s = &str[16];
    for (i=0; i<n; i++)
    {
	s += sprintf(s, "%d,", iov[i].IBU_IOV_LEN);
    }
    MPIDI_DBG_PRINTF((60, FCNAME, "%s\n", str));
#endif
    ibu->read.total = 0;
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*ibu->read.iov = iov;*/
    memcpy(ibu->read.iov, iov, sizeof(IBU_IOV) * n);
    ibu->read.iovlen = n;
    ibu->read.index = 0;
    ibu->read.use_iov = TRUE;
    ibu->read.progress_update = rfn;
    ibu->state |= IBU_READING;
    ibu->pending_operations++;
    /* copy any pre-received data into the iov */
    if (ibu->unex_list)
	ibui_readv_unex(ibu);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_READV);
    return IBU_SUCCESS;
}

#if 0
#undef FUNCNAME
#define FUNCNAME ibu_post_write
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_write(ibu_t ibu, void *buf, int len, int (*wfn)(int, void*))
{
    int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITE);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    /*
    ibu->write.total = 0;
    ibu->write.buffer = buf;
    ibu->write.bufflen = len;
    ibu->write.use_iov = FALSE;
    ibu->write.progress_update = wfn;
    ibu->state |= IBU_WRITING;
    ibu->pending_operations++;
    */
    ibu->state |= IBU_WRITING;

    num_bytes = ibui_post_write(ibu, buf, len, wfn);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITE);
    MPIDI_DBG_PRINTF((60, FCNAME, "returning %d\n", num_bytes));
    return num_bytes;
}
#endif

#if 0
#undef FUNCNAME
#define FUNCNAME ibu_post_writev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_post_writev(ibu_t ibu, IBU_IOV *iov, int n, int (*wfn)(int, void*))
{
    int num_bytes;
    MPIDI_STATE_DECL(MPID_STATE_IBU_POST_WRITEV);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_POST_WRITEV);
    MPIDI_DBG_PRINTF((60, FCNAME, "entering"));
    /* This isn't necessary if we require the iov to be valid for the duration of the operation */
    /*ibu->write.iov = iov;*/
    /*
    memcpy(ibu->write.iov, iov, sizeof(IBU_IOV) * n);
    ibu->write.iovlen = n;
    ibu->write.index = 0;
    ibu->write.use_iov = TRUE;
    ibu->write.progress_update = wfn;
    */
    ibu->state |= IBU_WRITING;
    /*
    {
	char str[1024], *s = str;
	int i;
	s += sprintf(s, "ibu_post_writev(");
	for (i=0; i<n; i++)
	    s += sprintf(s, "%d,", iov[i].IBU_IOV_LEN);
	sprintf(s, ")\n");
	MPIU_DBG_PRINTF(("%s", str));
    }
    */
    num_bytes = ibui_post_writev(ibu, iov, n, wfn);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_POST_WRITEV);
    return IBU_SUCCESS;
}
#endif

/* extended functions */

#undef FUNCNAME
#define FUNCNAME ibu_get_lid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int ibu_get_lid()
{
    MPIDI_STATE_DECL(MPID_STATE_IBU_GET_LID);

    MPIDI_FUNC_ENTER(MPID_STATE_IBU_GET_LID);
    MPIDI_FUNC_EXIT(MPID_STATE_IBU_GET_LID);
    return IBU_Process.lid;
}
