/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include "elan.h"
#include "elan_module.h"
#include "my_papi_defs.h"

/*
void mx__print_queue(MPID_nem_mx_req_queue_ptr_t qhead, int sens)
{
   MPID_nem_mx_cell_ptr_t curr = qhead->head;
   int index = 0;

   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
   
   while(curr != NULL)
     {
	fprintf(stdout,"[%i] -- [CELL %i @%p]: [REQUEST is %i @%p][NEXT @ %p] \n",
                MPID_nem_mem_region.rank,index,curr,curr->mx_request,&(curr->mx_request),curr->next);
	curr = curr->next;
	index++;
     }
   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
}
*/

#define TRACE 

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_elan_module_send_from_queue()
{
   MPID_nem_elan_cell_ptr_t elan_event_cell;
   int                      mpi_errno = MPI_SUCCESS;
   
   if (MPID_nem_module_elan_pendings_sends > 0)   
     {	
	if (elan_poll(MPID_nem_module_elan_pending_event_queue->head->elan_event,10) == TRUE)
	  {
#ifdef TRACE
	     fprintf(stdout,"[%i] ==== ELAN SEND FROM QUEUE : Done ====\n", MPID_nem_mem_region.rank);
#endif
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell);
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell->cell_ptr);	 
	     elan_event_cell->elan_event = NULL;
	     elan_event_cell->cell_ptr   = NULL;
	     MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell);
	     MPID_nem_module_elan_pendings_sends--;	     
	  }
     }
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}

//#undef TRACE

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_elan_module_recv()
{
   MPID_nem_cell_ptr_t  cell      = NULL;
   MPID_nem_pkt_t      *pkt       = NULL;   
   int                  mpi_errno = MPI_SUCCESS;
   char                *ptr       = NULL;
   char                *ptr2      = NULL;		       
   int                  size;
   int                  n_chunks;
   int                  index; 
   int                  slot_size = elan_queueMaxSlotSize(elan_base->state);

#ifdef TRACE
   fprintf(stdout,"[%i | nvpid %i] -- ELAN RECV : entering \n", MPID_nem_mem_region.rank,elan_base->state->vp);
#endif
   if (elan_queueRxPoll(rxq_ptr_array[MPID_nem_mem_region.rank],ELAN_POLL_EVENT) == TRUE )
     {	
	if (!MPID_nem_queue_empty(MPID_nem_module_elan_free_queue))
	  {	
	     MPID_nem_queue_dequeue (MPID_nem_module_elan_free_queue, &cell);
	     pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell);		     
	     ptr = elan_queueRxWait(rxq_ptr_array[MPID_nem_mem_region.rank],(char *)pkt,MPID_NEM_ELAN_LOOPS);		  
#ifdef TRACE 
	     fprintf(stdout,"[%i | nvpid %i] -- ELAN RECV : Wait 0 done  \n", MPID_nem_mem_region.rank,elan_base->state->vp);
#endif	     
	     //elan_queueRxComplete(rxq_ptr_array[MPID_nem_mem_region.rank]);	     
	    // MPIU_Assert((char* )ptr == (char *)pkt);
	     
	     size = MPID_NEM_CELL_DLEN(cell) + MPID_NEM_MPICH2_HEAD_LEN;
	     n_chunks = size/slot_size;
	     if (size%slot_size)
	       n_chunks++;
	     
#ifdef TRACE
	     fprintf(stdout,"[%i | nvpid %i] -- ELAN RECV : size %i \n", MPID_nem_mem_region.rank,elan_base->state->vp,size);
#endif
	     
	     for(index = 1 ; index < n_chunks ; index++)
	       {		  		       
		  
		  ptr = (char *)pkt + index*slot_size;
		  ptr2 = elan_queueRxWait(rxq_ptr_array[MPID_nem_mem_region.rank],ptr,MPID_NEM_ELAN_LOOPS);
#ifdef TRACE
		  fprintf(stdout,"[%i | nvpid %i] -- ELAN RECV : Wait %i done  \n", MPID_nem_mem_region.rank,elan_base->state->vp,index);		  		  
#endif	  
		  
		  //elan_queueRxComplete(rxq_ptr_array[MPID_nem_mem_region.rank]);		  		  
		  //MPIU_Assert((char* )ptr2 == (char *)ptr);
		  
	       }
	     elan_queueRxComplete(rxq_ptr_array[MPID_nem_mem_region.rank]);		  		  
	     MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);     	  
	  }
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_poll(MPID_nem_poll_dir_t in_or_out)
{
   int mpi_errno = MPI_SUCCESS;
   
   if (MPID_nem_elan_freq > 0)
     {	
	if (in_or_out == MPID_NEM_POLL_OUT)
	  {
	     MPID_nem_elan_module_send_from_queue();
	     MPID_nem_elan_module_recv();
	  }
	else
	  {
	     MPID_nem_elan_module_recv();
	     MPID_nem_elan_module_send_from_queue();
	  }
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}
