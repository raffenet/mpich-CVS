/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_SHM_H
#define MM_SHM_H

#include "mpidimpl.h"

int shm_init();
int shm_write(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int shm_read(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int shm_get_buffers(MPID_Request *request_ptr);
int shm_get_business_card(char *value);
int shm_cq_test();
int shm_can_connect(char *business_card);
int shm_post_connect(MPIDI_VC *vc_ptr, char *business_card);
int shm_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int shm_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);

#endif
