/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_VIA_RDMA_H
#define MM_VIA_RDMA_H

#include "mpidimpl.h"

int via_rdma_init();
int via_rdma_read(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int via_rdma_write(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int via_rdma_get_buffers(MPID_Request *request_ptr);
int via_rdma_get_business_card(char *value);
int via_rdma_cq_test();
int via_rdma_can_connect(char *business_card);
int via_rdma_post_connect(MPIDI_VC *vc_ptr, char *business_card);
int via_rdma_merge_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int via_rdma_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);

#endif
