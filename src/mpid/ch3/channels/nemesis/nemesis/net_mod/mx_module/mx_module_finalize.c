/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_module_impl.h"
#include "myriexpress.h"


int
MPID_nem_mx_module_finalize()
{
   while(MPID_nem_module_mx_pendings_sends > 0)
     {
	MPID_nem_mx_module_poll(MPID_NEM_POLL_OUT);
     }
   
   if (MPID_nem_mem_region.ext_procs > 0)
     {
	int ret ;
	ret = mx_close_endpoint(MPID_nem_module_mx_local_endpoint);
	if( ret != MX_SUCCESS)
	  FATAL_ERROR ("mx_close_endpoint failed");

	MPIU_Free( MPID_nem_module_mx_endpoints_addr );
	MPIU_Free( MPID_nem_module_mx_send_outstanding_request );      
	MPIU_Free( MPID_nem_module_mx_recv_outstanding_request );      
	
	ret = mx_finalize();
	if( ret != MX_SUCCESS)
	  FATAL_ERROR ("mx_finalize failed");
     }   
   return 0;
}

int
MPID_nem_mx_module_ckpt_shutdown ()
{
    return 0;
}

