#ifndef MODULE2_H
#define MODULE2_H
#include "mpid_nem.h"
#include <linux/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define TCP_POLL_FREQ_MULTI 5 
#define TCP_POLL_FREQ_ALONE 1
#define TCP_POLL_FREQ_NO   -1

typedef struct nodes_struct
{
  int                node_id; 
  int                desc;
  struct sockaddr_in sock_id;
  int                left2write;
  int                left2read_head; 
  int                left2read;
  int                toread;
  MPID_nem_queue_ptr_t     internal_recv_queue;
  MPID_nem_queue_ptr_t     internal_free_queue;
} node_t;

extern fd_set set;
extern int    max_fd;

extern int  ext_numnodes;
extern int *ext_ranks;
extern int numnodes;
extern int rank;
extern node_t *nodes;

extern int  n_pending_send;
extern int *n_pending_sends;
extern int  n_pending_recv;
extern int  outstanding;

extern int poll_freq;
extern int old_poll_freq;

extern MPID_nem_queue_ptr_t module_tcp_recv_queue;
extern MPID_nem_queue_ptr_t module_tcp_free_queue;

extern MPID_nem_queue_ptr_t process_recv_queue;
extern MPID_nem_queue_ptr_t process_free_queue;

#undef MPID_NEM_USE_MACROS
#ifndef MPID_NEM_USE_MACROS
static inline void
internal_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_ptr_t prev = qhead->tail;         
    
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
#ifndef MPID_NEM_USE_SHADOW_HEAD
static inline int 
internal_queue_empty ( MPID_nem_queue_ptr_t qhead )
{
  return qhead->head == NULL;
}
/* Gets the head */
static inline void 
internal_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
  register MPID_nem_cell_ptr_t _e = qhead->head ;
  
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
     *e = _e;
  }
}
#else //MPID_NEM_USE_SHADOW_HEAD
static inline int 
internal_queue_empty ( MPID_nem_queue_ptr_t qhead )
{
  if (qhead->my_head == NULL)
  {
      if (qhead->head == NULL)
      {
	  return 1;
      }
      else
      {
	  qhead->my_head = qhead->head;
	  qhead->head    = NULL; /* reset it for next time */
      }
  }
  return 0;
}
/* Gets the head */
static inline void 
internal_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
  register MPID_nem_cell_ptr_t _e = qhead->my_head ;
  
  if(_e == NULL)
  {    
    *e = NULL;
  }
  else
  {
     qhead->my_head  = _e->next;
     if(qhead->my_head == NULL)
     {  
        qhead->tail = NULL;  
     }
     _e->next = NULL;
     *e = _e;
  }
}
#endif //MPID_NEM_USE_SHADOW_HEAD
#else  //USE_MACROS
#define internal_queue_enqueue(qhead, element) do { \
    MPID_nem_cell_ptr_t prev = (qhead)->tail;              \
                                                    \
    if (prev == NULL)                               \
    {                                               \
        (qhead)->head = (element);                  \
    }                                               \
    else                                            \
    {                                               \
        prev->next = (element);                     \
    }                                               \
    (qhead)->tail = (element);                      \
} while (0) 
#ifndef MPID_NEM_USE_SHADOW_HEAD
#define internal_queue_empty(qhead) ((qhead)->head == NULL)
#define internal_queue_dequeue(qhead, e)    do { \
  register MPID_nem_cell_ptr_t _e = (qhead)->head ;     \
                                                 \
  if(_e == NULL)                                 \
  {                                              \
    *(e) = NULL;                                 \
  }                                              \
  else                                           \
  {                                              \
     (qhead)->head  = _e->next;                  \
     if((qhead)->head == NULL)                   \
     {                                           \
        (qhead)->tail = NULL;                    \
     }                                           \
     _e->next = NULL;                            \
     *(e) = _e;                                  \
  }                                              \
} while(0)                                       
#else //MPID_NEM_USE_SHADOW_HEAD
#define internal_queue_empty(qhead)      ({      \
  int __ret = 0;                                 \
  if ((qhead)->my_head == NULL)                  \
  {                                              \
      if ((qhead)->head == NULL)                 \
      {                                          \
	  __ret = 1;                             \
      }                                          \
      else                                       \
      {                                          \
	  (qhead)->my_head = (qhead)->head;      \
	  (qhead)->head    = NULL;               \
      }                                          \
  }                                              \
  __ret;                                         \
})
#define internal_queue_dequeue(qhead, e)  do {   \
  register MPID_nem_cell_ptr_t _e = (qhead)->my_head ;  \
                                                 \
  if(_e == NULL)                                 \
  {                                              \
    *(e) = NULL;                                 \
  }                                              \
  else                                           \
  {                                              \
     (qhead)->my_head  = _e->next;               \
     if((qhead)->my_head == NULL)                \
     {                                           \
        (qhead)->tail = NULL;                    \
     }                                           \
     _e->next = NULL;                            \
     *(e) = _e;                                  \
  }                                              \
} while(0)
#endif //MPID_NEM_USE_SHADOW_HEAD
#endif //USE_MACROS

#define MPID_NEM_USE_MACROS

#endif //MODULE2.H
