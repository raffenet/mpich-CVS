#ifndef DUM_MODULE_H
#define DUM_MODULE_H
#include "mpid_nem.h"

int dummy_module_init (MPID_nem_queue_ptr_t,
		       MPID_nem_queue_ptr_t,
		       MPID_nem_cell_ptr_t,
		       int,
		       MPID_nem_cell_ptr_t,
		       int,
		       MPID_nem_queue_ptr_t *,
		       MPID_nem_queue_ptr_t *,
			int ckpt_restart);
int  dummy_module_finalize();
int  dummy_module_ckpt_shutdown();
void dummy_module_poll (int);
void dummy_module_poll_send (void);
void dummy_module_poll_recv (void);
void dummy_module_send (int, MPID_nem_cell_ptr_t , int);

#endif /*DUN_MODULE.H */
