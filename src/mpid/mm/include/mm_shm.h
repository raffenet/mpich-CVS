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

#endif
