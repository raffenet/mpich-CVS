/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_TCP_H
#define MM_TCP_H

#include "mpidimpl.h"

int tcp_init();
int tcp_get_business_card(char *value, int length);
int tcp_can_connect(char *business_card);
int tcp_post_connect(MPIDI_VC *vc_ptr, char *business_card);
int tcp_post_read(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_merge_with_unexpected(MM_Car *car_ptr, MM_Car *unex_car_ptr);
int tcp_post_write(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_make_progress();
int tcp_car_enqueue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_car_dequeue(MPIDI_VC *vc_ptr, MM_Car *car_ptr);
int tcp_reset_car(MM_Car *car_ptr);

#endif
