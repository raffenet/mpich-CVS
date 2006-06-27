/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_module_impl.h"
#include "myriexpress.h"
#include "mx_module.h"
#include "my_papi_defs.h"

void  
MPID_nem_mx_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
   MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);
   
   if ( !MPID_nem_mx_req_queue_empty(MPID_nem_module_mx_send_free_req_queue))
     {			
	MPID_nem_mx_cell_ptr_t cell_req;
	mx_request_t          *request;   
	mx_return_t            ret;
	mx_segment_t           seg;
	mx_status_t            status;
	uint32_t               result;
	int                    dest = vc->lpid;
		
	MPID_nem_mx_req_queue_dequeue(MPID_nem_module_mx_send_free_req_queue,&cell_req);
	request = MPID_NEM_MX_CELL_TO_REQUEST(cell_req);		
	seg.segment_ptr    = (void *)(MPID_NEM_CELL_TO_PACKET (cell));
	seg.segment_length = datalen + MPID_NEM_MPICH2_HEAD_LEN;    
	
	ret = mx_isend(MPID_nem_module_mx_local_endpoint,
		       &seg,1,
		       MPID_nem_module_mx_endpoints_addr[dest],
		       MPID_NEM_MX_MATCH,
		       (void *)cell,
		       request);
	
	if(ret != MX_SUCCESS)
	  {	     
	     ERROR_RET (-1, "mx_isend() failed");
	  }	
	else
	  {	     
	     if(MPID_nem_module_mx_pendings_sends == 0)
	       {	
		  ret = mx_test(MPID_nem_module_mx_local_endpoint,
				request,
				&status,
				&result);
		  
		  if(ret != MX_SUCCESS)
		    {		       
		       ERROR_RET (-1, "mx_test() failed");
		    }
		  else
		    {		  
		       if((result != 0) && (status.code == MX_STATUS_SUCCESS))		    
			 {	
			    MPID_nem_queue_enqueue (MPID_nem_process_free_queue, (MPID_nem_cell_ptr_t)status.context);
			    MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_free_req_queue,cell_req);
			 }  
		       else
			 {
			    MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_pending_req_queue,cell_req);
			    MPID_nem_module_mx_pendings_sends++;
			 }  
		    }	     
	       }	
	     else
	       {
		  MPID_nem_mx_req_queue_enqueue(MPID_nem_module_mx_send_pending_req_queue,cell_req);
		  MPID_nem_module_mx_pendings_sends++;
	       }
	  }	
     }   
}
