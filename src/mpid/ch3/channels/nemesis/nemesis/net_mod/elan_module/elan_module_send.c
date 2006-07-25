/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include "elan.h"
#include "elan_module.h"
#include "my_papi_defs.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int 
MPID_nem_elan_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{   
   MPID_nem_elan_cell_ptr_t elan_event_cell;
   MPID_nem_pkt_t          *pkt       = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */
   int                      dest      = vc->lpid;
   int                      data_size = datalen + MPID_NEM_MPICH2_HEAD_LEN;
   int                      mpi_errno = MPI_SUCCESS;

   
   MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);

   fprintf(stdout,"[%i | nvpid %i] -- ELAN SEND to %i : trying %i bytes\n", MPID_nem_mem_region.rank,elan_base->state->vp,dest,data_size);

   if (!MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_free_event_queue))
     {	  
	MPID_nem_module_elan_free_event_queue->head->elan_event = elan_queueTx(rxq_ptr_array[dest],dest,pkt,(size_t) data_size,MPID_NEM_ELAN_RAIL_NUM);
	
	if (elan_poll(MPID_nem_module_elan_free_event_queue->head->elan_event,MPID_NEM_ELAN_LOOPS) == TRUE)
	  {
	     //fprintf(stdout,"[%i] -- ELAN SEND : Done \n", MPID_nem_mem_region.rank);
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue,cell);
	     MPID_nem_module_elan_free_event_queue->head->elan_event = NULL ;
	  }   
	else
	  {
	     //fprintf(stdout,"[%i] -- ELAN SEND : Pending \n", MPID_nem_mem_region.rank);
	     MPID_nem_module_elan_pendings_sends++;
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_free_event_queue,&elan_event_cell);
	     elan_event_cell->cell_ptr = cell ;
	     MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_pending_event_queue,elan_event_cell);	
	  }
     }
   else
     {
	MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell);
	elan_wait(elan_event_cell->elan_event,ELAN_WAIT_EVENT);
	MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell->cell_ptr);
	elan_event_cell->elan_event = NULL;	
	MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell);
	MPID_nem_module_elan_pendings_sends--;	
     }
              
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;   
}
