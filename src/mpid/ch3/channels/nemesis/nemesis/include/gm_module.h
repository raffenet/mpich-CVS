#ifndef GM_MODULE_H
#define GM_MODULE_H

int gm_module_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements,
		    int num_proc_elements, MPID_nem_cell_ptr_t module_elements, int num_module_elements,
		    MPID_nem_queue_ptr_t *module_recv_queue, MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart);
int gm_module_finalize( void );
int gm_module_ckpt_shutdown( void );
void gm_module_send (int dest, MPID_nem_cell_ptr_t cell, int datalen);
void gm_module_send_poll( void );
void gm_module_recv_poll( void );
void gm_module_poll( int );
int gm_module_test( void );

int gm_module_register_mem (void *p, int len);
int gm_module_deregister_mem (void *p, int len);

/* completion counter is atomically decremented when operation completes */
int gm_module_get (void *target_p, void *source_p, int len, int source_node, int *completion_ctr);
int gm_module_put (void *target_p, void *source_p, int len, int target_node, int *completion_ctr);

/* large message transfer functions */
int gm_module_lmt_send_pre (struct iovec *iov, size_t n_iov, int dest, struct iovec *cookie);
int gm_module_lmt_recv_pre (struct iovec *iov, size_t n_iov, int src, struct iovec *cookie);
int gm_module_lmt_start_send (int dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int gm_module_lmt_start_recv (int src, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int gm_module_lmt_send_post (struct iovec cookie);
int gm_module_lmt_recv_post (struct iovec cookie);

#define LMT_COMPLETE 0
#define LMT_FAILURE 1
#define LMT_AGAIN 2

#endif
