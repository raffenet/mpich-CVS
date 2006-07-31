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

#define MPID_NEM_ELAN_LOOPS     ELAN_POLL_EVENT
#define MPID_NEM_ELAN_RAIL_NUM  0
#define MPID_NEM_ELAN_SLOT_SIZE 2048
#define MPID_NEM_ELAN_NUM_SLOTS 1024

extern int             MPID_nem_elan_freq;
extern int             MPID_nem_elan_chunks;
extern int             MPID_nem_module_elan_pendings_sends;
extern int             MPID_nem_module_elan_pendings_recvs;
extern ELAN_QUEUE_TX **rxq_ptr_array;

typedef struct MPID_nem_elan_cell
{   
   struct MPID_nem_elan_cell *next;
   ELAN_EVENT                *elan_event;
   MPID_nem_cell_ptr_t        cell_ptr;
}
MPID_nem_elan_cell_t, *MPID_nem_elan_cell_ptr_t;

typedef struct MPID_nem_elan_event_queue
{   
   MPID_nem_elan_cell_ptr_t head;
   MPID_nem_elan_cell_ptr_t tail;
}
MPID_nem_elan_event_queue_t, *MPID_nem_elan_event_queue_ptr_t;

static inline int
MPID_nem_elan_event_queue_empty ( MPID_nem_elan_event_queue_ptr_t  qhead )
{   
   return qhead->head == NULL;
}

static inline void 
MPID_nem_elan_event_queue_enqueue (MPID_nem_elan_event_queue_ptr_t qhead, MPID_nem_elan_cell_ptr_t element)
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
MPID_nem_elan_event_queue_dequeue (MPID_nem_elan_event_queue_ptr_t qhead, MPID_nem_elan_cell_ptr_t *e)
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

extern MPID_nem_elan_cell_ptr_t        MPID_nem_module_elan_cells;
extern MPID_nem_elan_event_queue_ptr_t MPID_nem_module_elan_free_event_queue;
extern MPID_nem_elan_event_queue_ptr_t MPID_nem_module_elan_pending_event_queue;

extern MPID_nem_queue_ptr_t MPID_nem_module_elan_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_module_elan_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

#endif //ELAN_MODULE_IMPL_H
