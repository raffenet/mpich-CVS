#ifndef TCP_MODULE_H
#define TCP_MODULE_H
#include "mpid_nem.h"

int  MPID_nem_tcp_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		      MPID_nem_queue_ptr_t proc_free_queue, 
		      MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		      MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		      MPID_nem_queue_ptr_t *module_recv_queue,
		      MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		      MPIDI_PG_t *pg_p, int pg_rank,
		      char **bc_val_p, int *val_max_sz_p);
int  MPID_nem_tcp_module_finalize (void);
int MPID_nem_tcp_module_ckpt_shutdown (void);
void MPID_nem_tcp_module_poll (MPID_nem_poll_dir_t in_or_out);
void MPID_nem_tcp_module_poll_send (void);
void MPID_nem_tcp_module_poll_recv (void);
void MPID_nem_tcp_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int  MPID_nem_tcp_module_get_business_card (char **bc_val_p, int *val_max_sz_p);
int  MPID_nem_tcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int  MPID_nem_tcp_module_vc_init (MPIDI_VC_t *vc, const char *business_card);

/* completion counter is atomically decremented when operation completes */
int  MPID_nem_tcp_module_get (void *target_p, void *source_p, int source_node, int len, int *completion_ctr);
int  MPID_nem_tcp_module_put (void *target_p, int target_node, void *source_p, int len, int *completion_ctr);

/* large message transfer functions */
int MPID_nem_tcp_module_lmt_pre_send (MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV *cookie);
int MPID_nem_tcp_module_lmt_pre_recv (MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie, MPID_IOV *r_cookie, int *send_cts);
int MPID_nem_tcp_module_lmt_start_send (MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie);
int MPID_nem_tcp_module_lmt_start_recv (MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_tcp_module_lmt_post_send (MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_tcp_module_lmt_post_recv (MPIDI_VC_t *vc, MPID_Request *req);

#endif /*TCP_MODULE.H */
