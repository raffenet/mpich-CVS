#ifndef MPID_NEM_NETS_H
#define MPID_NEM_NETS_H

extern int  (* net_module_init) (MPID_nem_queue_ptr_t,
			  MPID_nem_queue_ptr_t,
			  MPID_nem_cell_ptr_t,
			  int,
			  MPID_nem_cell_ptr_t,
			  int,
			  MPID_nem_queue_ptr_t *,
			  MPID_nem_queue_ptr_t *,
			  int ckpt_restart);
extern int  (* net_module_finalize) (void);
extern int  (* net_module_ckpt_shutdown) (void);
extern void (* net_module_poll) (int);
extern void (* net_module_send) (int, MPID_nem_cell_ptr_t , int);


int MPID_nem_net_init(void);

#endif //MPID_NEM_NETS_H
