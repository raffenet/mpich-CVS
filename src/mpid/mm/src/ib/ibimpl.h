/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef IBIMPL_H
#define IBIMPL_H

#include "mm_ib.h"
#include "sock.h"

#define IB_EAGER_LIMIT       (1024 * 20)
#define IB_ERROR_MSG_LENGTH  256
#define IB_LISTENER_POINTER  &IB_Process

typedef struct IB_PerProcess {
    MPID_Thread_lock_t lock;
            sock_set_t set;
           sock_wait_t out;
                sock_t listener;
		   int port;
		  char host[100];
	    MPIDI_VC * read_list;
	    MPIDI_VC * write_list;
		   int error;
		  char err_msg[IB_ERROR_MSG_LENGTH];
} IB_PerProcess;

extern IB_PerProcess IB_Process;

int ib_handle_read(MPIDI_VC *vc_ptr, int num_bytes);
int ib_handle_read_data(MPIDI_VC *vc_ptr, int num_read);
int ib_handle_read_ack(MPIDI_VC *vc_ptr, int num_read);
int ib_handle_read_connect_pkt(MPIDI_VC *vc_ptr, int num_read);
int ib_handle_written(MPIDI_VC *vc_ptr, int num_bytes);
int ib_handle_written_ack(MPIDI_VC *vc_ptr, int num_written);
int ib_handle_written_connect_pkt(MPIDI_VC *vc_ptr, int num_written);
/*
int ib_read_data(MPIDI_VC *vc_ptr);
int ib_write_aggressive(MPIDI_VC *vc_ptr);
*/

#endif
