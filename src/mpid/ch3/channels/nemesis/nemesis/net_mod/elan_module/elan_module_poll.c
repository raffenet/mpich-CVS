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

//#define TRACE 

inline int 
MPID_nem_elan_module_send_from_queue_no_test()
{

}

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
	ELAN_EVENT              *elan_event_ptr  = NULL;
	MPID_nem_elan_cell_ptr_t elan_event_cell = NULL;
	
	elan_event_ptr = MPID_nem_module_elan_pending_event_queue->head->elan_event ;
	while( (elan_event_ptr != NULL) && (elan_poll(elan_event_ptr,MPID_NEM_ELAN_LOOPS_SEND) == TRUE))
	  {
	     MPID_nem_elan_event_queue_dequeue(MPID_nem_module_elan_pending_event_queue,&elan_event_cell);
#ifdef TRACE
	     if (MPID_nem_mem_region.rank == 0)
	       fprintf(stdout,"[%i] ==== ELAN SEND FROM QUEUE : Done with SEQNO %i====\n",
		       MPID_nem_mem_region.rank,MPID_NEM_CELL_SEQN(elan_event_cell->cell_ptr));
#endif
	     
	     MPID_nem_queue_enqueue (MPID_nem_process_free_queue,elan_event_cell->cell_ptr);	 
	     elan_event_cell->elan_event = NULL;
	     elan_event_cell->cell_ptr   = NULL;
	     MPID_nem_elan_event_queue_enqueue(MPID_nem_module_elan_free_event_queue,elan_event_cell);	   
	     if ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
	       elan_event_ptr = MPID_nem_module_elan_pending_event_queue->head->elan_event ;
	     else
	       elan_event_ptr = NULL ;
	  }	
     }
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;   
}

//#undef TRACE

static int expecting = 0;
//static int my__seqno = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
inline int 
MPID_nem_elan_module_recv()
{
   MPID_nem_cell_ptr_t  cell      = NULL;
   MPID_nem_pkt_t      *pkt       = NULL;   
   int                  res       = FALSE;
   int                  mpi_errno = MPI_SUCCESS;
   
#ifdef TRACE
   fprintf(stdout,"[%i | nvpid %i] ================ ELAN RECV : entering (exp %i) ============== \n", MPID_nem_mem_region.rank,elan_base->state->vp,expecting);
#endif

   if(!expecting)
     res = elan_queueRxPoll(rxq_ptr_array[MPID_nem_mem_region.rank],MPID_NEM_ELAN_LOOPS_RECV);
   
   if(( res == TRUE ) || (expecting > 0)) 
     {	
#ifdef TRACE
	fprintf(stdout,"[%i | nvpid %i] -- ELAN RECV : entering 2\n", MPID_nem_mem_region.rank,elan_base->state->vp);
#endif
	if (!MPID_nem_queue_empty(MPID_nem_module_elan_free_queue))
	  {	
	     MPID_nem_queue_dequeue (MPID_nem_module_elan_free_queue, &cell);
	     pkt = (MPID_nem_pkt_t *)MPID_NEM_CELL_TO_PACKET (cell);
	     elan_queueRxWait(rxq_ptr_array[MPID_nem_mem_region.rank],(char *)pkt,ELAN_POLL_EVENT);
	     
	     /*
	      ptr = elan_queueRxWait(rxq_ptr_array[MPID_nem_mem_region.rank],(char *)pkt,ELAN_POLL_EVENT);
	      MPIU_Assert((char* )ptr == (char *)pkt);	     
	     size = MPID_NEM_CELL_DLEN(cell) + MPID_NEM_MPICH2_HEAD_LEN;

#ifdef TRACE
	     if (MPID_nem_mem_region.rank == 1)
	       fprintf(stdout,"[%i | nvpid %i] -- ELAN RECV : size %i \n\t\t-- ELAN RECV : dest %i \n\t\t-- ELAN RECV : seqno %i \n", 
		     MPID_nem_mem_region.rank,elan_base->state->vp,size,MPID_NEM_CELL_DEST(cell),MPID_NEM_CELL_SEQN(cell));
#endif	
	      MPIU_Assert(my__seqno == MPID_NEM_CELL_SEQN(cell));
	     my__seqno++;	     
	     elan_queueRxComplete(rxq_ptr_array[MPID_nem_mem_region.rank]);		  		  
	     */
	     
	     MPID_nem_queue_enqueue (MPID_nem_process_recv_queue, cell);	     	     
	     if(expecting)
	       expecting--;
	  }
	else
	  {
	     if(!expecting)
	       expecting++;
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
