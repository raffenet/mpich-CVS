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
void elan__print_queue(MPID_nem_elan_event_queue_ptr_t qhead, int sens)
{
   MPID_nem_elan_cell_ptr_t curr = qhead->head;
   int index = 0;

   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
   
   while(curr != NULL)
     {
	fprintf(stdout,"[%i] -- [ELAN_CELL %i is @ %p]: [EVENT is @%p][CELL PTR is @ %p][NEXT is %p] \n",
                MPID_nem_mem_region.rank,index,curr,curr->elan_event,curr->cell_ptr,curr->next);
	curr = curr->next;
	index++;
     }
   if(sens)
     fprintf(stdout,"=======================ENQUEUE=========================== \n");
   else
     fprintf(stdout,"=======================DEQUEUE=========================== \n");
}
*/

/*#define TRACE*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_send_from_queue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int
MPID_nem_elan_module_send_from_queue()
{   
   int mpi_errno = MPI_SUCCESS;
   
   if ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
     {	
	ELAN_EVENT              *elan_event_ptr   = NULL;
	MPID_nem_elan_cell_ptr_t elan_event_cell  = NULL;
	
	elan_event_cell = MPID_nem_module_elan_pending_event_queue->head ;  	
	while(1)
	  {
	     if(elan_event_cell->to_proceed)
	       {
		  MPID_nem_pkt_t *pkt  = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET(elan_event_cell->cell_ptr);
		  int             dest = MPID_NEM_CELL_DEST(elan_event_cell->cell_ptr);
		  
		  elan_event_cell->elan_event = 
		    elan_queueTx(rxq_ptr_array[dest],dest,(char *)pkt,(size_t)(MPID_NEM_PACKET_LEN(pkt)),MPID_NEM_ELAN_RAIL_NUM);
		  /*
		   #ifdef TRACE
		   fprintf(stdout,"[%i] ================ ELAN SEND FROM QUEUE : EVENT CREATED %p  ============= \n", MPID_nem_mem_region.rank,elan_event_cell->elan_event);
		   #endif      
		   */ 
		  if(elan_poll(elan_event_cell->elan_event,MPID_NEM_ELAN_LOOPS_SEND) == TRUE)
		    {
		       MPID_nem_elan_cell_ptr_t elan_event_cell2 = NULL;
		       /*
			#ifdef TRACE
			fprintf(stdout,"[%i] ================ ELAN SEND FROM QUEUE : POLL OK  ============= \n", MPID_nem_mem_region.rank);
			#endif
			*/ 
		       MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell2);
		       MPID_NEM_ELAN_RESET_CELL( elan_event_cell2 );
		       MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell2);		       		       
		       
		       if ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
			 elan_event_cell = MPID_nem_module_elan_pending_event_queue->head ;
		       else
			 goto fn_exit;
		    }
		  else
		    {
		       /*
			#ifdef TRACE
			fprintf(stdout,"[%i] ================ ELAN SEND FROM QUEUE : POLL NOT OK  ============= \n", MPID_nem_mem_region.rank);
			#endif
			*/ 
		       elan_event_cell->to_proceed = 0;
		       goto fn_exit;
		    }		  
	       }
	     else
	       {		  
		  if(elan_poll(elan_event_cell->elan_event,MPID_NEM_ELAN_LOOPS_SEND) == TRUE)
		    {
		       MPID_nem_elan_cell_ptr_t elan_event_cell2 = NULL;
		       
		       MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell2);
		       /*
			#ifdef TRACE
			fprintf(stdout,"[%i] ==== ELAN SEND FROM QUEUE : Done with SEQNO %i====\n",
			MPID_nem_mem_region.rank,MPID_NEM_CELL_SEQN(elan_event_cell2->cell_ptr));
			#endif
			*/ 		       
		       MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell2->cell_ptr);	 
		       MPID_NEM_ELAN_RESET_CELL( elan_event_cell2 );
		       MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell2);
		       
		       if ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
			 elan_event_cell = MPID_nem_module_elan_pending_event_queue->head ;
		       else
			 goto fn_exit;
		    }
		  else
		    {
		       /*
			#ifdef TRACE
			fprintf(stdout,"[%i] ================ ELAN SEND FROM QUEUE : POLL NOT OK 2  ============= \n", MPID_nem_mem_region.rank);
			#endif
			*/ 
		       goto fn_exit;
		    }		  
	       }
	  }
     }

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}


/*static unsigned short prev__seqno = 0;*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_elan_module_recv()
{
   int mpi_errno = MPI_SUCCESS;
   
   if((!MPID_nem_queue_empty(MPID_nem_module_elan_free_queue)) && 
      (elan_queueRxPoll(rxq_ptr_array[MPID_nem_mem_region.rank],MPID_NEM_ELAN_LOOPS_RECV) == TRUE ))
     {
	MPID_nem_cell_ptr_t  cell = NULL;
	MPID_nem_pkt_t      *pkt  = NULL;   
	/*char                *ptr  = NULL ;*/
	/*	
	 #ifdef TRACE
	 fprintf(stdout,"[%i] ================ ELAN RECV : QUEUE POLL DONE : got msg ============== \n", MPID_nem_mem_region.rank);
	 #endif
	 */
	
	MPID_nem_queue_dequeue (MPID_nem_module_elan_free_queue, &cell);
	pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell);	     	     	     
	/*ptr = */ 
	elan_queueRxWait(rxq_ptr_array[MPID_nem_mem_region.rank],(char *)pkt,ELAN_WAIT_EVENT);	     	     
	/* elan_queueRxComplete(rxq_ptr_array[MPID_nem_mem_region.rank]); */
	/*
	 #ifdef TRACE
	 MPIU_Assert((char* )ptr == (char *)pkt);
	 fprintf(stdout,"[%i] -- ELAN RECV : size %i \n\t\t-- ELAN RECV : dest %i \n\t\t-- ELAN RECV : seqno %i \n", 
	 MPID_nem_mem_region.rank,(MPID_NEM_CELL_DLEN(cell) + MPID_NEM_MPICH2_HEAD_LEN),MPID_NEM_CELL_DEST(cell),MPID_NEM_CELL_SEQN(cell));
	 if(MPID_NEM_CELL_SEQN(cell) > 0)
	 {
	 if( (prev__seqno+1) != MPID_NEM_CELL_SEQN(cell))
	 {
	 MPIU_Assert( (prev__seqno+1) != MPID_NEM_CELL_SEQN(cell));
	 }
	 prev__seqno = MPID_NEM_CELL_SEQN(cell);
	 }
	 else
	 {
	 MPIU_Assert( MPID_NEM_CELL_SEQN(cell) == 0);
	 }
	 #endif
	 */ 
	MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	     	     
     }	
   /*
    else
    {
    #ifdef TRACE
    fprintf(stdout,"[%i] >>>>>>>>>>>>>>>>>>>> ELAN RECV : ELAN FREE Q EMPTY or res NULL  >>>>>>>>>>>>>>>>>>>>\n", MPID_nem_mem_region.rank);
    #endif	     
    }
    */
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
