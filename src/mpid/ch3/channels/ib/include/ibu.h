/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IBU_H
#define IBU_H

#ifdef __cplusplus
extern "C" {
#endif

/* config header file */
#include "mpidi_ch3i_ib_conf.h"
#include "mpichconf.h"
#include "mpiiov.h"

#ifdef MPID_IBU_TYPE_WINDOWS
#include <winsock2.h>
#include <windows.h>
#include <limits.h>
#endif

#ifdef USE_IB_VAPI

#define IBU_MAX_PINNED (32*1024*1024)

#include <vapi.h>
#include <vapi_common.h>
#include <mpga.h>
typedef VAPI_cq_hndl_t ibu_set_t;
typedef struct ibu_mem_t
{
    VAPI_mr_hndl_t handle;
    VAPI_lkey_t lkey;
    VAPI_rkey_t rkey;
} ibu_mem_t;

#elif defined(USE_IB_IBAL)

#define IBU_MAX_PINNED (32*1024*1024)

#include <ib_al.h>
typedef ib_cq_handle_t ibu_set_t;
typedef struct ibu_mem_t
{
    int m,l,r;
} ibu_mem_t;

#else
#error No infiniband access layer specified
#endif


/* definitions */

typedef enum ibu_op_e
{
    IBU_OP_TIMEOUT,
    IBU_OP_READ,
    IBU_OP_WRITE,
    IBU_OP_CLOSE,
    IBU_OP_WAKEUP
} ibu_op_t;

/* insert error codes here */
#define IBU_SUCCESS           0
#define IBU_FAIL              -1
#define IBU_ERR_TIMEOUT       1001



/* definitions/structures specific to Windows */
#ifdef MPID_IBU_TYPE_WINDOWS

#define IBU_INFINITE        INFINITE
#define IBU_INVALID_QP      0

/* definitions/structures specific to Unix */
#elif defined(MPID_IBU_TYPE_UNIX)

#define IBU_INFINITE        -1
#define IBU_INVALID_QP      0

#else
#error Error: MPID_IBU_TYPE not defined
#endif

typedef struct ibu_state_t * ibu_t;


/* function prototypes */
int ibu_init(void);
int ibu_finalize(void);

int ibu_get_lid();

int ibu_create_set(ibu_set_t *set);
int ibu_destroy_set(ibu_set_t set);

/*int ibu_set_user_ptr(ibu_t ibu, void *user_ptr);*/
int ibu_set_vc_ptr(ibu_t ibu, void *vc_ptr);

ibu_t ibu_start_qp(ibu_set_t set, int *qp_num_ptr);
int ibu_finish_qp(ibu_t ibu, int dest_lid, int dest_qpnum);
int ibu_post_read(ibu_t ibu, void *buf, int len);
int ibu_post_readv(ibu_t ibu, MPID_IOV *iov, int n);
int ibu_write(ibu_t ibu, void *buf, int len, int *num_bytes_ptr);
int ibu_writev(ibu_t ibu, MPID_IOV *iov, int n, int *num_bytes_ptr);

/*int ibu_wait(ibu_set_t set, int millisecond_timeout, ibu_wait_t *out);*/
int ibu_wait(ibu_set_t set, int millisecond_timeout, void **vc_pptr, int *num_bytes_ptr, ibu_op_t *op_ptr);

int ibu_register_memory(void *buf, int len, ibu_mem_t *state);
int ibu_deregister_memory(void *buf, int len, ibu_mem_t *state);

#ifdef __cplusplus
}
#endif

#endif
