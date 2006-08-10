/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SCTP_MODULE_IMPL_H
#define SCTP_MODULE_IMPL_H
#include "mpid_nem_impl.h"
#include "sctp_module.h"
#include "all_hash.h"
/* #include <linux/types.h> */ /* not needed for SCTP */
/* #include <sys/types.h>  */ /* not needed for SCTP */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <fcntl.h>  /* new for SCTP */
#include <errno.h>
#include <netdb.h> /* needed for gethostbyname */
#include <arpa/inet.h> /* needed for inet_pton  */

#define SCTP_POLL_FREQ_MULTI 5 
#define SCTP_POLL_FREQ_ALONE 1
#define SCTP_POLL_FREQ_NO   -1
#define SCTP_END_STRING "NEM_SCTP_MOD_FINALIZE"

extern MPID_nem_queue_ptr_t MPID_nem_sctp_module_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;

extern int MPID_nem_sctp_onetomany_fd; /* needed for all communication */
extern int MPID_nem_sctp_port; /* needed for init and bizcards */

/* hash table entry */
typedef struct hash_entry
{
    sctp_assoc_t assoc_id;
    MPIDI_VC_t * vc;
} MPID_nem_sctp_hash_entry;

/* hash table for association ID -> VC */
extern HASH* MPID_nem_sctp_assocID_table;


/* determine the stream # of a req. copied from UBC sctp channel so could be irrelevant */
/*    we want to use context so that collectives don't
 *    cause head-of-line blocking for p2p...
 */
 /* the following keeps stream s.t. 0 <= stream < MPICH_SCTP_NUM_STREAMS */
#define Req_Stream_from_match(match) (abs((match.tag) + (match.context_id))% MPICH_SCTP_NUM_STREAMS)
#define REQ_Stream(req) Req_Stream_from_match(req->dev.match)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq);


int MPID_nem_sctp_module_send_progress();


extern MPID_nem_queue_ptr_t MPID_nem_module_sctp_free_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_recv_queue;
extern MPID_nem_queue_ptr_t MPID_nem_process_free_queue;   

/* below are from the original cp of tcp net mod so they might be irrelevant */

/* #undef MPID_NEM_USE_MACROS */
/* #ifndef MPID_NEM_USE_MACROS */
/* static inline void */
/* MPID_nem_sctp_internal_queue_enqueue (internal_queue_t *qhead, MPID_nem_cell_ptr_t element) */
/* { */
/*     MPID_nem_abs_cell_ptr_t abs_element = (MPID_nem_abs_cell_ptr_t)element; */
/*     MPID_nem_abs_cell_ptr_t prev = qhead->tail;          */
    
/*     if (prev == NULL) */
/*     { */
/*         qhead->head = abs_element; */
/*     } */
/*     else */
/*     { */
/*         prev->next = abs_element; */
/*     } */
/*     qhead->tail = abs_element; */
/* } */

/* static inline int  */
/* MPID_nem_sctp_internal_queue_empty (const internal_queue_t qhead) */
/* { */
/*     return qhead.head == NULL; */
/* } */
/* /\* Gets the head *\/ */
/* static inline void  */
/* MPID_nem_sctp_internal_queue_dequeue (internal_queue_t *qhead, MPID_nem_cell_ptr_t *e) */
/* { */
/*     register MPID_nem_abs_cell_ptr_t _e = qhead->head; */
  
/*     if(_e == NULL) */
/*     { */
/* 	*e = NULL; */
/*     } */
/*     else */
/*     { */
/* 	qhead->head  = _e->next; */
/* 	if(qhead->head == NULL) */
/* 	{   */
/* 	    qhead->tail = NULL;   */
/* 	} */
/* 	_e->next = NULL; */
/* 	*e = (MPID_nem_cell_ptr_t)_e; */
/*     } */
/* } */
/* #else  /\*USE_MACROS *\/ */

/* #define MPID_nem_sctp_internal_queue_enqueue(qhead, element) do {		\ */
/*     MPID_nem_abs_cell_ptr_t abs_element = (MPID_nem_abs_cell_ptr_t)(element);	\ */
/*     MPID_nem_cell_ptr_t prev = (qhead)->tail;					\ */
/* 										\ */
/*     if (prev == NULL)								\ */
/*     {										\ */
/*         (qhead)->head = abs_element;						\ */
/*     }										\ */
/*     else									\ */
/*     {										\ */
/*         prev->next = abs_element;						\ */
/*     }										\ */
/*     (qhead)->tail = abs_element;						\ */
/* } while (0)  */

/* #define MPID_nem_sctp_internal_queue_empty(qhead) ((qhead).head == NULL) */

/* #define MPID_nem_sctp_internal_queue_dequeue(qhead, e)    do {	\ */
/*     register MPID_nem_cell_ptr_t _e = (qhead)->head;	\ */
/*     							\ */
/*     if(_e == NULL)					\ */
/*     {							\ */
/*         *(e) = NULL;					\ */
/*     }							\ */
/*     else						\ */
/*     {							\ */
/*         (qhead)->head  = _e->next;			\ */
/*         if((qhead)->head == NULL)			\ */
/*         {						\ */
/*             (qhead)->tail = NULL;			\ */
/*         }						\ */
/*         _e->next = NULL;				\ */
/*         *(e) = (MPID_nem_cell_ptr_t)_e;			\ */
/*     }							\ */
/* } while(0)                                        */
/* #endif /\* USE_MACROS *\/ */


/* #define MPID_NEM_USE_MACROS */
#endif /* SCTP_MODULE_IMPL_H */
