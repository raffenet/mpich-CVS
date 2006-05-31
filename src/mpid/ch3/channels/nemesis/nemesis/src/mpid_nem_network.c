/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem.h"
#if  (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
#include "gm_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
#include "tcp_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
#include "dummy_module.h"
#else
#warning ">>>>>>>>>>>>>>>> WRONG NET MODULE SELECTION"
#endif 

int  (* MPID_nem_net_module_init) (MPID_nem_queue_ptr_t proc_recv_queue, 
				   MPID_nem_queue_ptr_t proc_free_queue, 
				   MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
				   MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
				   MPID_nem_queue_ptr_t *module_recv_queue,
				   MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
				   MPIDI_PG_t *pg_p, int pg_rank,
				   char **bc_val_p, int *val_max_sz_p);
int  (* MPID_nem_net_module_finalize) (void);
int  (* MPID_nem_net_module_ckpt_shutdown) (void);
void (* MPID_nem_net_module_poll) (MPID_nem_poll_dir_t in_or_out);
void (* MPID_nem_net_module_send) (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int (* MPID_nem_net_module_get_business_card) (char **bc_val_p, int *val_max_sz_p);
int (* MPID_nem_net_module_connect_to_root) (const char *business_card, MPIDI_VC_t *new_vc);
int (* MPID_nem_net_module_vc_init) (MPIDI_VC_t *vc, const char *business_card);

#define assign_functions(prefix) do {						          \
    MPID_nem_net_module_init              = MPID_nem_##prefix##_module_init;	          \
    MPID_nem_net_module_finalize          = MPID_nem_##prefix##_module_finalize;          \
    MPID_nem_net_module_ckpt_shutdown     = MPID_nem_##prefix##_module_ckpt_shutdown;	  \
    MPID_nem_net_module_poll              = MPID_nem_##prefix##_module_poll;		  \
    MPID_nem_net_module_send              = MPID_nem_##prefix##_module_send;		  \
    MPID_nem_net_module_get_business_card = MPID_nem_##prefix##_module_get_business_card; \
    MPID_nem_net_module_connect_to_root   = MPID_nem_##prefix##_module_connect_to_root;	  \
    MPID_nem_net_module_vc_init           = MPID_nem_##prefix##_module_vc_init;		  \
} while (0)

int
MPID_nem_net_init( void)
{
#if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
  {
      assign_functions (gm);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
  {
      assign_functions (tcp);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
  {
      assign_functions (dummy);
  }
#else
#warning ">>>>>>>>>>>>>>>> WRONG NET MODULE INITIALIZATION"
#endif 
  return 0;
}
