#ifndef DUM_MODULE_H
#define DUM_MODULE_H
#include "mpid_nem.h"

int  MPID_nem_dummy_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
				 MPID_nem_queue_ptr_t proc_free_queue, 
				 MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
				 MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
				 MPID_nem_queue_ptr_t *module_recv_queue,
				 MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart, MPIDI_PG_t *pg_p);
int  MPID_nem_dummy_module_finalize();
int  MPID_nem_dummy_module_ckpt_shutdown();
void MPID_nem_dummy_module_poll (MPID_nem_poll_dir_t in_or_out);
void MPID_nem_dummy_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int  MPID_nem_dummy_module_get_business_card (char **bc_val_p, int *val_max_sz_p);
int  MPID_nem_dummy_module_connect_to_root (const char *business_card, const int lpid);
int  MPID_nem_dummy_module_vc_init (MPIDI_VC_t *vc);

#endif /*DUM_MODULE.H */
