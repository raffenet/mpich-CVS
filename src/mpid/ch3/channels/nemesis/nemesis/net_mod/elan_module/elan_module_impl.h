/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ELAN_MODULE_IMPL_H
#define ELAN_MODULE_IMPL_H
#include <elan.h>
#include "mpid_nem_impl.h"
#include <linux/types.h>

extern int MPID_nem_module_elan_pendings_sends;
extern int MPID_nem_module_elan_pendings_recvs;

/*
static inline int
MPID_nem_elan_req_queue_empty ( MPID_nem_elan_req_queue_ptr_t  qhead )
{   
   return qhead->head == NULL;
}

static inline void 
MPID_nem_elan_req_queue_enqueue (MPID_nem_elan_req_queue_ptr_t qhead, MPID_nem_elan_cell_ptr_t element)
{   
   MPID_nem_elan_cell_ptr_t prev = qhead->tail;   
   if (prev == NULL)
     {	
	qhead->head = element;
     }   
   else
     {	
	prev->next = element;
     }   
   qhead->tail = element;
}

static inline void
MPID_nem_elan_req_queue_dequeue (MPID_nem_elan_req_queue_ptr_t qhead, MPID_nem_elan_cell_ptr_t *e)
{   
   register MPID_nem_elan_cell_ptr_t _e = qhead->head;   
   if(_e == NULL)
     {	
	*e = NULL;
     }   
   else
     {	
	qhead->head  = _e->next;
	if(qhead->head == NULL)
	  {	     
	     qhead->tail = NULL;
	  }	
	_e->next = NULL;
	*e = (MPID_nem_elan_cell_ptr_t)_e;
     }   
}
*/
extern MPID_nem_queue_ptr_t MPID_nem_module_elan_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_module_elan_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

#endif //ELAN_MODULE_IMPL_H
