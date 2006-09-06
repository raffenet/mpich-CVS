/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef _IB_MODULE_IMPL_H
#define _IB_MODULE_IMPL_H

#include "mpid_nem.h"
#include "ib_utils.h"
#include <infiniband/verbs.h>

#define FREE_SEND_QUEUE_ELEMENTS  MPID_NEM_NUM_CELLS

extern MPID_nem_queue_ptr_t MPID_nem_module_ib_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_module_ib_free_queue;

extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

extern struct ibv_mr *proc_elements_mr;
extern struct ibv_mr *module_elements_mr;

typedef struct {
    union {
        struct ibv_send_wr s_wr;
        struct ibv_recv_wr r_wr;
    } u;
    struct ibv_sge sg_list;
} MPID_nem_ib_module_descriptor_t;

typedef struct {
    MPID_nem_cell_ptr_t     nem_cell;
    int                     datalen;
    MPID_nem_ib_module_queue_elem_t *qe;
    MPIDI_VC_t              *vc;
    MPID_nem_ib_module_descriptor_t desc;
} MPID_nem_ib_module_cell_elem_t;

typedef struct _ib_cell_pool {
    MPID_nem_ib_module_queue_t          *queue;
    int                                 ncells;
} MPID_nem_ib_module_cell_pool_t;

extern MPID_nem_ib_module_cell_pool_t MPID_nem_ib_module_cell_pool;

int MPID_nem_ib_module_init_cell_pool(int n);
int MPID_nem_ib_module_get_cell(
        MPID_nem_ib_module_cell_elem_t **e);
void MPID_nem_ib_module_return_cell(
        MPID_nem_ib_module_cell_elem_t *ce);
void MPID_nem_ib_module_prep_cell_recv(
        MPID_nem_ib_module_cell_elem_t *ce,
        void* buf);
void MPID_nem_ib_module_prep_cell_send(
        MPID_nem_ib_module_cell_elem_t *ce,
        void* buf, uint32_t len);
int MPID_nem_ib_module_add_cells(int n);

#endif /* _IB_MODULE_IMPL_H */
