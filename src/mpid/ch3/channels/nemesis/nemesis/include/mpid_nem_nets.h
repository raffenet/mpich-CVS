/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_NETS_H
#define MPID_NEM_NETS_H

extern int  (* MPID_nem_net_module_init) (MPID_nem_queue_ptr_t proc_recv_queue, 
					  MPID_nem_queue_ptr_t proc_free_queue, 
					  MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
					  MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
					  MPID_nem_queue_ptr_t *module_recv_queue,
					  MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
					  MPIDI_PG_t *pg_p, int pg_rank,
					  char **bc_val_p, int *val_max_sz_p);     
extern int  (* MPID_nem_net_module_finalize) (void);
extern int  (* MPID_nem_net_module_ckpt_shutdown) (void);
extern void (* MPID_nem_net_module_poll) (MPID_nem_poll_dir_t in_or_out);
extern void (* MPID_nem_net_module_send) (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
extern int (* MPID_nem_net_module_get_business_card) (char **bc_val_p, int *val_max_sz_p);
extern int (* MPID_nem_net_module_connect_to_root) (const char *business_card, MPIDI_VC_t *new_vc);
extern int (* MPID_nem_net_module_vc_init) (MPIDI_VC_t *vc, const char *business_card);

int MPID_nem_net_init(void);

#endif //MPID_NEM_NETS_H
