/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TCP_MODULE_IMPL_H
#define TCP_MODULE_IMPL_H

extern MPID_nem_queue_ptr_t MPID_nem_tcp_module_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_tcp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;   

extern int MPID_nem_tcp_module_listen_fd = 0;


#endif /* TCP_MODULE_IMPL_H */
