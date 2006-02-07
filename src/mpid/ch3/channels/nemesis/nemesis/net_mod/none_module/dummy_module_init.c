#include "mpid_nem.h"

int
dummy_module_init (MPID_nem_queue_ptr_t  proc_recv_queue, 
		   MPID_nem_queue_ptr_t  proc_free_queue, 
		   MPID_nem_cell_ptr_t   proc_elements,   
		   int                   num_proc_elements,
		   MPID_nem_cell_ptr_t   module_elements, 
		   int                   num_module_elements, 
		   MPID_nem_queue_ptr_t *module_recv_queue,
		   MPID_nem_queue_ptr_t *module_free_queue,
		   int ckpt_restart, MPIDI_PG_t *pg_p)
{
    return 0;
}




