/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_TCP_H
#define MM_TCP_H

#include "mpidimpl.h"

int tcp_init();
int tcp_get_business_card(char *value);
int tcp_post_read();
int tcp_post_write();
int tcp_cq_test();

#endif
