/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_VIA_H
#define MM_VIA_H

#include "mpidimpl.h"

int via_init();
int via_read(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int via_write(struct MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int via_get_buffers(MPID_Request *request_ptr);
int via_get_business_card(char *value);
int via_cq_test();
int via_can_connect(char *business_card);
int via_post_connect(MPIDI_VC *vc_ptr, char *business_card);

#endif
