#ifndef TCP_MODULE_IMPL_H
#define TCP_MODULE_IMPL_H
#include "mpid_nem.h"
#include <linux/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define TCP_POLL_FREQ_MULTI 5 
#define TCP_POLL_FREQ_ALONE 1
#define TCP_POLL_FREQ_NO   -1

typedef struct MPID_nem_tcp_module_internal_queue
{
    MPID_nem_abs_cell_ptr_t     head;
    MPID_nem_abs_cell_ptr_t     tail;
} internal_queue_t, * volatile internal_queue_ptr_t;

typedef struct nodes_struct
{
    int                  node_id; 
    int                  desc;
    struct sockaddr_in   sock_id;
    int                  left2write;
    int                  left2read_head; 
    int                  left2read;
    int                  toread;
    internal_queue_ptr_t internal_recv_queue;
    internal_queue_ptr_t internal_free_queue;
} node_t;

typedef struct tcp_internal
{
   fd_set  set;
   int     max_fd;
   int     ext_numnodes;
   int    *ext_ranks;
   node_t *nodes;
   int     n_pending_send;
   int    *n_pending_sends;
   int     n_pending_recv;
   int     outstanding;
   int     poll_freq;
   int     old_poll_freq;
} mpid_nem_tcp_internal_t ;

extern mpid_nem_tcp_internal_t MPID_nem_tcp_internal_vars;

extern MPID_nem_queue_ptr_t module_tcp_recv_queue;
extern MPID_nem_queue_ptr_t module_tcp_free_queue;
extern MPID_nem_queue_ptr_t process_recv_queue;
extern MPID_nem_queue_ptr_t process_free_queue;   


#undef MPID_NEM_USE_MACROS
#ifndef MPID_NEM_USE_MACROS
static inline void
MPID_nem_tcp_internal_queue_enqueue (internal_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_abs_cell_ptr_t abs_element = (MPID_nem_abs_cell_ptr_t)element;
    MPID_nem_abs_cell_ptr_t prev = qhead->tail;         
    
    if (prev == NULL)
    {
        qhead->head = abs_element;
    }
    else
    {
        prev->next = abs_element;
    }
    qhead->tail = abs_element;
}

static inline int 
MPID_nem_tcp_internal_queue_empty ( internal_queue_ptr_t qhead )
{
    return qhead->head == NULL;
}
/* Gets the head */
static inline void 
MPID_nem_tcp_internal_queue_dequeue (internal_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    register MPID_nem_abs_cell_ptr_t _e = qhead->head;
  
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
	*e = (MPID_nem_cell_ptr_t)_e;
    }
}
#else  /*USE_MACROS */

#define MPID_nem_tcp_internal_queue_enqueue(qhead, element) do {		\
    MPID_nem_abs_cell_ptr_t abs_element = (MPID_nem_abs_cell_ptr_t)(element);	\
    MPID_nem_cell_ptr_t prev = (qhead)->tail;					\
										\
    if (prev == NULL)								\
    {										\
        (qhead)->head = abs_element;						\
    }										\
    else									\
    {										\
        prev->next = abs_element;						\
    }										\
    (qhead)->tail = abs_element;						\
} while (0) 

#define MPID_nem_tcp_internal_queue_empty(qhead) ((qhead)->head == NULL)

#define MPID_nem_tcp_internal_queue_dequeue(qhead, e)    do {	\
    register MPID_nem_cell_ptr_t _e = (qhead)->head;	\
    							\
    if(_e == NULL)					\
    {							\
        *(e) = NULL;					\
    }							\
    else						\
    {							\
        (qhead)->head  = _e->next;			\
        if((qhead)->head == NULL)			\
        {						\
            (qhead)->tail = NULL;			\
        }						\
        _e->next = NULL;				\
        *(e) = (MPID_nem_cell_ptr_t)_e;			\
    }							\
} while(0)                                       
#endif /* USE_MACROS */


#define MPID_NEM_USE_MACROS
#endif /* TCP_MODULE_IMPL_H */
