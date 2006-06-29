/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem.h"
#if  (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
#include "gm_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_MX_MODULE)
#include "mx_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_TCP_MODULE)
#include "tcp_module.h"
#elif(MPID_NEM_NET_MODULE == MPID_NEM_NO_MODULE)
#include "dummy_module.h"
#else
#warning ">>>>>>>>>>>>>>>> WRONG NET MODULE SELECTION"
#endif 

MPID_nem_net_module_init_t MPID_nem_net_module_init = 0;
MPID_nem_net_module_finalize_t MPID_nem_net_module_finalize = 0;
MPID_nem_net_module_ckpt_shutdown_t MPID_nem_net_module_ckpt_shutdown = 0;
MPID_nem_net_module_poll_t MPID_nem_net_module_poll = 0;
MPID_nem_net_module_send_t MPID_nem_net_module_send = 0;
MPID_nem_net_module_get_business_card_t MPID_nem_net_module_get_business_card = 0;
MPID_nem_net_module_connect_to_root_t MPID_nem_net_module_connect_to_root = 0;
MPID_nem_net_module_vc_init_t MPID_nem_net_module_vc_init = 0;

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

#undef FUNCNAME
#define FUNCNAME MPID_nem_net_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_net_init( void)
{
    int mpi_errno = MPI_SUCCESS;
    
#if (MPID_NEM_NET_MODULE == MPID_NEM_GM_MODULE)
  {
      assign_functions (gm);
  }
#elif (MPID_NEM_NET_MODULE == MPID_NEM_MX_MODULE)
  {
      assign_functions (mx);
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
  return mpi_errno;
}
