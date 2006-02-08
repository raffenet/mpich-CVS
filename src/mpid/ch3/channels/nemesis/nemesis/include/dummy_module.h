#ifndef DUM_MODULE_H
#define DUM_MODULE_H
#include "mpid_nem.h"

int dummy_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		       MPID_nem_queue_ptr_t proc_free_queue, 
		       MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		       MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		       MPID_nem_queue_ptr_t *module_recv_queue,
		       MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart, MPIDI_PG_t *pg_p);
int  dummy_module_finalize();
int  dummy_module_ckpt_shutdown();
void dummy_module_poll (MPID_nem_poll_dir_t in_or_out);
void dummy_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int dummy_module_get_business_card (char **bc_val_p, int *val_max_sz_p);
int dummy_module_connect_to_root (const char *business_card, const int lpid);
int dummy_module_vc_init (MPIDI_VC_t *vc);

#endif /*DUN_MODULE.H */
