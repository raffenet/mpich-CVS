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
   MPID_nem_pkt_t          *pkt        = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell); /* cast away volatile */
   int                      dest       = vc->lpid;
   int                      data_size  = datalen + MPID_NEM_MPICH2_HEAD_LEN;
   int                      mpi_errno  = MPI_SUCCESS;
   int                      slot_size  = elan_queueMaxSlotSize(elan_base->state);
   
   MPIU_Assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);
   fprintf(stdout,"[%i | nvpid %i] ==== ELAN SEND to %i : trying %i bytes  ==== \n", MPID_nem_mem_region.rank,elan_base->state->vp,dest,data_size);
  
   /* Does data fits into a single Slot ? */
   if (data_size <= slot_size)
     {
	if (!MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_free_event_queue))
	  {	  
	     previous_step:
	       {		  
		  MPID_nem_module_elan_free_event_queue->head->elan_event = 
		    elan_queueTx(rxq_ptr_array[dest],dest,(char *)pkt,(size_t)data_size,MPID_NEM_ELAN_RAIL_NUM);
		  
		  if (elan_poll(MPID_nem_module_elan_free_event_queue->head->elan_event,1) == TRUE)
		    {
		       //fprintf(stdout,"[%i] ==== ELAN SEND : Done ====\n", MPID_nem_mem_region.rank);
		       MPID_nem_queue_enqueue (MPID_nem_process_free_queue,cell);
		       MPID_nem_module_elan_free_event_queue->head->elan_event = NULL ;
		    }	     
		  else
		    {
		       //		  fprintf(stdout,"[%i] -- ELAN SEND : Pending \n", MPID_nem_mem_region.rank);
		       MPID_nem_module_elan_pendings_sends++;
		       MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_free_event_queue,&elan_event_cell);
		       elan_event_cell->cell_ptr = cell ;
		       MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_pending_event_queue,elan_event_cell);	
		    }
	       }	     
	  }
	else
	  {
	     fprintf(stdout,"[%i] -- ELAN SEND : Queue Empty !!! \n", MPID_nem_mem_region.rank);
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell);
	     elan_wait(elan_event_cell->elan_event,ELAN_WAIT_EVENT);
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell->cell_ptr);
	     elan_event_cell->elan_event = NULL;
	     elan_event_cell->cell_ptr   = NULL ;
	     MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell);
	     MPID_nem_module_elan_pendings_sends--;
	     goto previous_step;
	  }	
     }
   else
     {
	ELAN_EVENT *current_event = NULL;
	char       *ptr    = (char *)pkt;
	int         chunks = 0;
	int         size;  
	int         index ;
	
	chunks = data_size/slot_size;
	if (data_size%slot_size)
	  chunks++;
	
	fprintf(stdout,"[%i | nvpid %i] ==== ELAN SEND to %i : trying %i bytes in %i chunks ==== \n", MPID_nem_mem_region.rank,elan_base->state->vp,dest,data_size,chunks);
	
	for(index = 0; index < chunks; index++)
	  {	     
	     ptr = (char *)pkt+ index*slot_size;	     
	     if(index == (chunks - 1))
	       size = data_size%slot_size;
	     else
	       size = slot_size; 	
	     
	     if (!MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_free_event_queue))
	       {
		  previous_step2:
		    {		       
		       MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_free_event_queue,&elan_event_cell);		  
		       elan_event_cell->elan_event = 
			 elan_queueTx(rxq_ptr_array[dest],dest,(char *)ptr,(size_t)size,MPID_NEM_ELAN_RAIL_NUM);		  
		       current_event = elan_link(current_event,elan_event_cell->elan_event);
		       
		       if (index == (chunks - 1))
			 {		       
			    if (elan_poll(current_event,1) == TRUE)
			      {
				 fprintf(stdout,"[%i] ==== ELAN SEND : Done ====\n", MPID_nem_mem_region.rank);
				 elan_event_cell->elan_event = NULL;
				 elan_event_cell->cell_ptr   = NULL;			    
				 MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell);			    
				 MPID_nem_queue_enqueue (MPID_nem_process_free_queue,cell);
			      }	     
			    else
			      {
				 fprintf(stdout,"[%i] -- ELAN SEND : Pending for chunk %i\n", MPID_nem_mem_region.rank,index);
				 elan_event_cell->elan_event = current_event;
				 elan_event_cell->cell_ptr   = cell ;			    
				 MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_pending_event_queue,elan_event_cell);
				 MPID_nem_module_elan_pendings_sends++;			    
			      }
			 }
		    }		  
	       }	     
	     else
	       {
		  fprintf(stdout,"[%i] -- ELAN SEND : Queue Empty !!! \n", MPID_nem_mem_region.rank);
		  MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell);
		  elan_wait(elan_event_cell->elan_event,ELAN_WAIT_EVENT);
		  MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell->cell_ptr);
		  elan_event_cell->elan_event = NULL;	
		  elan_event_cell->cell_ptr   = NULL ;
		  MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell);
		  MPID_nem_module_elan_pendings_sends--;
		  goto previous_step2;
	       }
	  }   
     }
   
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;   
}
