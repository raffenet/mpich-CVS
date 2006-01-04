#ifndef MPID_NEM_QUEUE_H
#define MPID_NEM_QUEUE_H
#include "mpid_nem_datatypes.h"
#include "mpid_nem_defs.h"
#include "mpid_nem_atomics.h"
/*#include "papi.h" */

/*#define PAPI_MONITOR */
#include "my_papi_defs.h"

#define MPID_nem_dump_cell_mpich2(cell, index)  __MPID_nem_dump_cell_mpich2((cell),(index),__FILE__,__LINE__) 

void __MPID_nem_dump_cell_mpich2 ( MPID_nem_cell_ptr_t cell, int, char* ,int);
inline void   MPID_nem_dump_cell_mpich ( MPID_nem_cell_ptr_t cell, int);

inline void MPID_nem_cell_init( MPID_nem_cell_ptr_t cell);
static inline void MPID_nem_dump_queue( MPID_nem_queue_ptr_t q) {printf ("dump queue not implemented\n"); exit (-1);};
inline void MPID_nem_queue_init( MPID_nem_queue_ptr_t );
void MPID_nem_network_poll (int in_or_out);

inline void MPID_nem_rel_cell_init( MPID_nem_cell_ptr_t cell);
static inline void MPID_nem_rel_dump_queue( MPID_nem_queue_ptr_t q){printf ("dump queue not implemented\n"); exit (-1);};
inline void MPID_nem_rel_queue_init( MPID_nem_queue_ptr_t );
void MPID_nem_rel_network_poll (int in_or_out);

#define MPID_NEM_USE_SHADOW_HEAD

#define MPID_NEM_USE_MACROS

#ifndef MPID_NEM_USE_MACROS
static inline void
MPID_nem_queue_enqueue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_cell_ptr_t   prev;

    prev = MPID_NEM_SWAP (&(qhead->tail), element);

    if (prev == NULL)
    {
	qhead->head = element;
    }
    else
    {
	prev->next  = element;
    }
}
#else /*MPID_NEM_USE_MACROS */
#define MPID_nem_queue_enqueue(qhead, element) do {	\
    MPID_nem_cell_ptr_t prev;				\
						\
    prev = MPID_NEM_SWAP (&((qhead)->tail), (element));	\
    if (prev == NULL)				\
    {						\
	(qhead)->head = (element);		\
    }						\
    else					\
    {						\
	prev->next = (element);			\
    }						\
} while (0)
#endif /*MPID_NEM_USE_MACROS */

#ifndef MPID_NEM_USE_MACROS
static inline void
MPID_nem_rel_queue_enqueue (MPID_nem_queue_ptr_t rel_qhead, MPID_nem_cell_ptr_t element)
{
    MPID_nem_queue_ptr_t qhead = MPID_NEM_REL_TO_ABS( rel_qhead );
    MPID_nem_cell_ptr_t   prev;

    prev = MPID_NEM_SWAP (&(qhead->tail), element);

    if (prev == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
    {
	qhead->head = element;
    }
    else
    {
        (MPID_NEM_REL_TO_ABS(prev))->next = element;
    }
}
#else /*MPID_NEM_USE_MACROS */
#define MPID_nem_rel_queue_enqueue(rel_qhead, element) do {	\
    MPID_nem_queue_ptr_t qhead = MPID_NEM_REL_TO_ABS((MPID_nem_queue_ptr_t)rel_qhead ); \
    MPID_nem_cell_ptr_t prev;				\
						\
    prev = MPID_NEM_SWAP (&((qhead)->tail), (element));	\
    if (prev == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))	\
    {						\
	(qhead)->head = (element);		\
    }						\
    else					\
    {						\
        (MPID_NEM_REL_TO_ABS(prev))->next = (element);   \
    }						\
} while (0)
#endif /*MPID_NEM_USE_MACROS */

#ifndef MPID_NEM_USE_MACROS
static inline int
MPID_nem_queue_empty ( MPID_nem_queue_ptr_t qhead )
{
#ifdef MPID_NEM_USE_SHADOW_HEAD
    if (qhead->my_head == NULL)
    {
	if (qhead->head == NULL)
	    return 1;
	else
	{
	    qhead->my_head = qhead->head;
	    qhead->head = NULL; /* reset it for next time */
	}
    }
    return 0;
#else
    return qhead->head == NULL;
#endif
}
#else /*MPID_NEM_USE_MACROS */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#define MPID_nem_queue_empty(qhead) ({			\
    int __ret = 0;				\
    if (qhead->my_head == NULL)			\
    {						\
	if (qhead->head == NULL)		\
	    __ret = 1;				\
	else					\
        {					\
	    qhead->my_head = qhead->head;	\
	    qhead->head = NULL;			\
        }					\
    }						\
						\
    __ret;					\
})
#else
#define MPID_nem_queue_empty(qhead) ((qhead)->head == NULL)
#endif /* MPID_NEM_USE_SHADOW_HEAD */
#endif /* MPID_NEM_USE_MACROS */


#ifndef MPID_NEM_USE_MACROS
static inline int
MPID_nem_rel_queue_empty ( MPID_nem_queue_ptr_t rel_qhead )
{     
    MPID_nem_queue_ptr_t  qhead =  MPID_NEM_REL_TO_ABS( rel_qhead );

#ifdef MPID_NEM_USE_SHADOW_HEAD
    if (qhead->my_head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
    {
	if (qhead->head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
	    return 1;
	else
	{
	    qhead->my_head = qhead->head;
	    qhead->head = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL); /* reset it for next time */
	}
    }
    return 0;
#else /*  MPID_NEM_USE_SHADOW_HEAD */
    return qhead->head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);
#endif /*   MPID_NEM_USE_SHADOW_HEAD */
}
#else /*MPID_NEM_USE_MACROS */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#define MPID_nem_rel_queue_empty(rel_qhead) ({                                 \
    MPID_nem_queue_ptr_t  qhead =  MPID_NEM_REL_TO_ABS( (MPID_nem_queue_ptr_t)rel_qhead ); \
    int             __ret = 0;			                      \
    if (qhead->my_head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))                \
    {						                      \
	if (qhead->head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))               \
	    __ret = 1;				                      \
	else					                      \
        {					                      \
	    qhead->my_head = qhead->head;	                      \
	    qhead->head = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);                \
        }					                      \
    }						                      \
						                      \
    __ret;					                      \
})
#else /*MPID_NEM_USE_SHADOW_HEAD */
#define MPID_nem_rel_queue_empty(qhead) ((MPID_NEM_REL_TO_ABS((qhead)))->head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
#endif /* MPID_NEM_USE_SHADOW_HEAD */
#endif /* MPID_NEM_USE_MACROS */

#ifndef MPID_NEM_USE_MACROS
/* Gets the head */
static inline void
MPID_nem_queue_dequeue (MPID_nem_queue_ptr_t qhead, MPID_nem_cell_ptr_t *e)
{
    register MPID_nem_cell_ptr_t _e;
    /*DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); */
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet)); */

#ifdef MPID_NEM_USE_SHADOW_HEAD
    (_e) = qhead->my_head;
#else /*SHADOW_HEAD */
    (_e) = qhead->head ;
#endif /*SHADOWHEAD */

    if ((_e)->next != NULL)
    {
#ifdef MPID_NEM_USE_SHADOW_HEAD
      qhead->my_head  = (_e)->next;
#else /*MPID_NEM_USE_SHADOW_HEAD */
      qhead->head     = (_e)->next;
#endif /*MPID_NEM_USE_SHADOW_HEAD */
    }
    else
    {
	MPID_nem_cell_ptr_t old_tail;
      
#ifdef MPID_NEM_USE_SHADOW_HEAD
	qhead->my_head = NULL;	
#else /*MPID_NEM_USE_SHADOW_HEAD */
	qhead->head = NULL;	
#endif /*MPID_NEM_USE_SHADOW_HEAD */

	old_tail = MPID_NEM_CAS (&(qhead->tail), (_e), NULL);

	if (old_tail != (_e))
	{
	    while ((_e)->next == NULL)
	    {
		SKIP;
	    }
#ifdef MPID_NEM_USE_SHADOW_HEAD
      qhead->my_head  = (_e)->next;
#else /*MPID_NEM_USE_SHADOW_HEAD */
      qhead->head  = (_e)->next;
#endif /*MPID_NEM_USE_SHADOW_HEAD */
	}
    }
    (_e)->next   = NULL;
    *e = _e;
    /*    DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); */
}

#else /* MPID_NEM_USE_MACROS */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#define MPID_nem_queue_dequeue(qhead, e) do {						\
    register MPID_nem_cell_ptr_t _e;							\
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet));	*/				\
    /*(_e) = (qhead)->head;*/							\
    (_e) = (qhead)->my_head;							\
    if ((_e)->next != NULL)							\
    {										\
	/*(qhead)->head  = (_e)->next;*/					\
	(qhead)->my_head  = (_e)->next;						\
    }										\
    else									\
    {										\
	MPID_nem_cell_ptr_t old_tail;							\
										\
	/*(qhead)->head  = NULL;*/						\
	(qhead)->my_head  = NULL;						\
	old_tail = MPID_NEM_CAS (&((qhead)->tail), (_e), NULL);				\
	if (old_tail != (_e))							\
	{									\
	    while ((_e)->next == NULL)						\
	    {									\
		SKIP;								\
	    }									\
	    /*(qhead)->head = (_e)->next;*/					\
	    (qhead)->my_head = (_e)->next;					\
	}									\
    }										\
										\
    (_e)->next   = NULL;							\
    *(e) = _e;									\
    /*    DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11));*/	\
    /*DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11)); */	\
} while (0)                                             
#else /* MPID_NEM_USE_SHADOW_HEAD */
#define MPID_nem_queue_dequeue(qhead, e) do {						\
    register MPID_nem_cell_ptr_t _e;							\
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet));	*/				\
    (_e) = (qhead)->head;							\
    if ((_e)->next != NULL)							\
    {										\
	(qhead)->head  = (_e)->next;						\
    }										\
    else									\
    {										\
	MPID_nem_cell_ptr_t old_tail;							\
										\
	(qhead)->head  = NULL;							\
	old_tail = MPID_NEM_CAS (&((qhead)->tail), (_e), NULL);				\
	if (old_tail != (_e))							\
	{									\
	    while ((_e)->next == NULL)						\
	    {									\
		SKIP;								\
	    }									\
	    (qhead)->head = (_e)->next;						\
	}									\
    }										\
										\
    (_e)->next   = NULL;							\
    *(e) = _e;									\
    /*    DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11));*/	\
    /*DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11)); */	\
} while (0)                                             

#endif /* MPID_NEM_USE_SHADOW_HEAD */
#endif /* MPID_NEM_USE_MACROS */


#ifndef MPID_NEM_USE_MACROS
/* Gets the head */
static inline void
MPID_nem_rel_queue_dequeue (MPID_nem_queue_ptr_t rel_qhead, MPID_nem_cell_ptr_t *e)
{
    MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( rel_qhead );
    register MPID_nem_cell_ptr_t _e;    
    register MPID_nem_cell_ptr_t _e_abs;
    
    /*DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); */
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet)); */

#ifdef MPID_NEM_USE_SHADOW_HEAD
    (_e) = qhead->my_head;
#else /*SHADOW_HEAD */
    (_e) = qhead->head ;
#endif /*SHADOWHEAD */

    (_e_abs) = MPID_NEM_REL_TO_ABS( (_e) );
    if ((_e_abs)->next != ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
    {
#ifdef MPID_NEM_USE_SHADOW_HEAD
      qhead->my_head  = (_e_abs)->next;
#else /*MPID_NEM_USE_SHADOW_HEAD */
      qhead->head     = (_e_abs)->next;
#endif /*MPID_NEM_USE_SHADOW_HEAD */
    }
    else
    {
	MPID_nem_cell_ptr_t old_tail;
      
#ifdef MPID_NEM_USE_SHADOW_HEAD
	qhead->my_head = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);	
#else /*MPID_NEM_USE_SHADOW_HEAD */
	qhead->head = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);	
#endif /*MPID_NEM_USE_SHADOW_HEAD */

	old_tail = MPID_NEM_CAS (&(qhead->tail), (_e), ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL));

	if (old_tail != (_e))
	{
	    while ((_e_abs)->next == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
	    {
		SKIP;
	    }
#ifdef MPID_NEM_USE_SHADOW_HEAD
      qhead->my_head  = (_e_abs)->next;
#else /*MPID_NEM_USE_SHADOW_HEAD */
      qhead->head  = (_e_abs)->next;
#endif /*MPID_NEM_USE_SHADOW_HEAD */
	}
    }
    (_e_abs)->next   = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);
    *e = _e;
    /*    DO_PAPI (if (iter11) PAPI_accum_var (PAPI_EventSet, PAPI_vvalues11)); */
}

#else /* MPID_NEM_USE_MACROS */
#ifdef MPID_NEM_USE_SHADOW_HEAD
#define MPID_nem_rel_queue_dequeue(rel_qhead, e) do {					\
    MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( (MPID_nem_queue_ptr_t)rel_qhead );            \
    register MPID_nem_cell_ptr_t _e;							\
    register MPID_nem_cell_ptr_t _e_abs;		       				\
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet));	*/				\
    (_e)     = (qhead)->my_head;		     				\
    (_e_abs) = MPID_NEM_REL_TO_ABS( (_e) );                                              \
    if ((_e_abs)->next != ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))				\
    {										\
	(qhead)->my_head  = (_e_abs)->next;			      		\
    }										\
    else									\
    {										\
	MPID_nem_cell_ptr_t old_tail;							\
										\
	(qhead)->my_head  = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);			\
	old_tail = MPID_NEM_CAS (&((qhead)->tail), (_e), ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL)); 	\
	if (old_tail != (_e))							\
	{									\
	    while ((_e_abs)->next == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))		\
	    {									\
		SKIP;								\
	    }									\
	    (qhead)->my_head = (_e_abs)->next;					\
	}									\
    }										\
										\
    (_e_abs)->next   = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);		      		\
    *(e) = _e;									\
    /*    DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11));*/	\
    /*DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11)); */	\
} while (0)                                             
#else /* MPID_NEM_USE_SHADOW_HEAD */
#define MPID_nem_rel_queue_dequeue(rel_qhead, e) do {				        \
    MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( (MPID_nem_queue_ptr_t)rel_qhead );            \
    register MPID_nem_cell_ptr_t _e;                                                   \
    register MPID_nem_cell_ptr_t _e_abs;		       		      	        \
    /*    DO_PAPI (PAPI_reset (PAPI_EventSet));	*/				\
    (_e)     = (qhead)->head;                                                   \
    (_e_abs) = MPID_NEM_REL_TO_ABS( (_e) );                                              \
    if ((_e_abs)->next != ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))				\
    {										\
	(qhead)->head  = (_e_abs)->next;				      	\
    }										\
    else									\
    {										\
	MPID_nem_cell_ptr_t old_tail;							\
										\
	(qhead)->head  = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);	         	  	\
	old_tail = MPID_NEM_CAS (&((qhead)->tail), (_e), ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL));   \
	if (old_tail != (_e))							\
	{									\
	    while ((_e_abs)->next == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))     		\
	    {									\
		SKIP;								\
	    }									\
	    (qhead)->head = (_e_abs)->next;		      			\
	}									\
    }										\
										\
    (_e_abs)->next   = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);			      	\
    *(e) = _e;									\
    /*    DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11));*/	\
    /*DO_PAPI (if (iter11) PAPI_accum (PAPI_EventSet, PAPI_values11)); */	\
} while (0)                                             

#endif /* MPID_NEM_USE_SHADOW_HEAD */
#endif /* MPID_NEM_USE_MACROS */

#ifdef MPID_NEM_USE_SHADOW_HEAD
static inline 
void MPID_nem_queue_poll (MPID_nem_queue_ptr_t qhead, int in_or_out)
{
    MPID_nem_network_poll (in_or_out);
    if (qhead->my_head == NULL)
    {
	while (qhead->head == NULL)
	{
	    MPID_nem_network_poll ( in_or_out );
	    sched_yield();
	}
	qhead->my_head = qhead->head;	
	qhead->head = NULL;   
    }
}
#else 
static inline void MPID_nem_queue_poll (MPID_nem_queue_ptr_t qhead, int in_or_out)
{
    MPID_nem_network_poll ( in_or_out );    
    while (qhead->head == NULL)
    {
	MPID_nem_network_poll ( in_or_out );
	sched_yield();
    }
}
#endif/* MPID_NEM_USE_SHADOW_HEAD */

#ifdef MPID_NEM_USE_SHADOW_HEAD
static inline 
void MPID_nem_rel_queue_poll (MPID_nem_queue_ptr_t rel_qhead, int in_or_out)
{
    MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( rel_qhead );

    MPID_nem_rel_network_poll (in_or_out);
    if (qhead->my_head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
    {
	while (qhead->head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
	{
	    MPID_nem_rel_network_poll ( in_or_out );
	    sched_yield();
	}
	qhead->my_head = qhead->head;	
	qhead->head = ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL);   
    }
}
#else 
static inline void MPID_nem_rel_queue_poll (MPID_nem_queue_ptr_t rel_qhead, int in_or_out)
{
    MPID_nem_queue_ptr_t  qhead = MPID_NEM_REL_TO_ABS( rel_qhead );

    MPID_nem_rel_network_poll ( in_or_out );    
    while (qhead->head == ((MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL))
    {
	MPID_nem_rel_network_poll ( in_or_out );
	sched_yield();
    }
}
#endif/* MPID_NEM_USE_SHADOW_HEAD */

#endif /* MPID_NEM_QUEUE_H */
