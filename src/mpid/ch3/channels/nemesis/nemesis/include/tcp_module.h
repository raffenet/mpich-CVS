#ifndef TCP_MODULE_H
#define TCP_MODULE_H
#include "mpid_nem.h"

int  tcp_module_init (MPID_nem_queue_ptr_t,
		      MPID_nem_queue_ptr_t,
		      MPID_nem_cell_ptr_t,
		      int,
		      MPID_nem_cell_ptr_t,
		      int,
		      MPID_nem_queue_ptr_t *,
		      MPID_nem_queue_ptr_t *,
		      int ckpt_restart);
int  tcp_module_finalize();
int tcp_module_ckpt_shutdown();
void tcp_module_poll (int);
void tcp_module_poll_send (void);
void tcp_module_poll_recv (void);
void tcp_module_send (int, MPID_nem_cell_t *, int);

/* completion counter is atomically decremented when operation completes */
int tcp_module_get (void *target_p, void *source_p, int source_node, int len, int *completion_ctr);
int tcp_module_put (void *target_p, int target_node, void *source_p, int len, int *completion_ctr);

#endif /*TCP_MODULE.H */
