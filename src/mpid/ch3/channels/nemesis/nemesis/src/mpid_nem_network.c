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

int  (* MPID_nem_net_module_init) (MPID_nem_queue_ptr_t,
				   MPID_nem_queue_ptr_t,
				   MPID_nem_cell_ptr_t,
				   int,
				   MPID_nem_cell_ptr_t,
				   int,
				   MPID_nem_queue_ptr_t *,
				   MPID_nem_queue_ptr_t *,
				   int ckpt_restart);
int  (* MPID_nem_net_module_finalize) (void);
int  (* MPID_nem_net_module_ckpt_shutdown) (void);
void (* MPID_nem_net_module_poll) (int);
void (* MPID_nem_net_module_send) (int, MPID_nem_cell_ptr_t , int);

int
MPID_nem_net_init( void)
{
#if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
  {
    MPID_nem_net_module_init          = gm_module_init; 
    MPID_nem_net_module_finalize      = gm_module_finalize;
    MPID_nem_net_module_ckpt_shutdown = gm_module_ckpt_shutdown; 
    MPID_nem_net_module_poll          = gm_module_poll;
    MPID_nem_net_module_send          = gm_module_send ;
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
  {
    MPID_nem_net_module_init          = tcp_module_init; 
    MPID_nem_net_module_finalize      = tcp_module_finalize;
    MPID_nem_net_module_ckpt_shutdown = tcp_module_ckpt_shutdown; 
    MPID_nem_net_module_poll          = tcp_module_poll;
    MPID_nem_net_module_send          = tcp_module_send ;
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
  {
    MPID_nem_net_module_init          = dummy_module_init; 
    MPID_nem_net_module_finalize      = dummy_module_finalize;
    MPID_nem_net_module_ckpt_shutdown = dummy_module_ckpt_shutdown; 
    MPID_nem_net_module_poll          = dummy_module_poll;
    MPID_nem_net_module_send          = dummy_module_send ;
  }
#else
#warning ">>>>>>>>>>>>>>>> WRONG NET MODULE INITIALIZATION"
#endif 
  return 0;
}
