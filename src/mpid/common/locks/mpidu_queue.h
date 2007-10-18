/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_QUEUE_H_INCLUDED
#define MPIDU_QUEUE_H_INCLUDED

#include "mpidu_atomic_primitives.h"

/* This value is used to indicate NULL in the MPIDU_Shm_asymm_base_addr
   variable.  It is non-zero because one of the likely base addresses is zero
   (indicating memory is actually symmetrically mapped).  The value 64 was used
   because it is unlikely that mmap will choose an address this low for a
   mapping.  */
#define MPIDU_SHM_ASYMM_NULL_VAL    64

extern char *MPIDU_Shm_asymm_base_addr;

/* Used to initialize the base address for relative pointers.  This interface
   assumes that there is only one shared memory segment.  If this turns out to
   not be the case in the future, we should probably add support for multiple
   shm segments.
   
   This function will return an error if it has already been called. */
int MPIDU_Shm_asymm_init(char *base);

/* Relative addressing macros.  These are for manipulating addresses relative
   to the start of a shared memory region. */
#define MPIDU_SHM_REL_NULL (0x0)
#define MPIDU_SHM_IS_REL_NULL(rel_ptr) ((rel_ptr).offset == MPIDU_SHM_REL_NULL)
#define MPIDU_SHM_SET_REL_NULL(rel_ptr) ((rel_ptr).offset = MPIDU_SHM_REL_NULL)
#define MPIDU_SHM_REL_ARE_EQUAL(rel_ptr1, rel_ptr2) ((rel_ptr1).offset == (rel_ptr2).offset)

/* This structure exists such that it is possible to expand the expressiveness
   of a relative address at some point in the future.  It also provides a
   modicum of type safety to help prevent certain flavors of errors.
   
   For example, instead of referencing an offset from a global base address, it
   might make sense for there to be multiple base addresses.  These base
   addresses could correspond to the start of a segment or region of shared
   memory.  This structure could store the segment number that is used to lookup
   a base address in a non-shared table.  Note that you would have to be very
   careful about all of this because if you add the segment number as a separate
   field you can no longer (compare and) swap a relative address atomically.  So
   you'll either have to shave bits from the pointer or make some sort of
   requirement that relative addresses can only be swapped within the same
   segment.  */
typedef struct MPIDU_Shm_rel_addr_t {
    MPI_Aint offset;
} MPIDU_Shm_rel_addr_t;

/* converts a relative pointer to an absolute pointer */
static inline
void *MPIDU_Shm_rel_to_abs(MPIDU_Shm_rel_addr_t r)
{
    return (void*)(MPID_Shm_asymm_base_addr + r.offset);
}

/* converts an absolute pointer to a relative pointer */
static inline
MPIDU_Shm_rel_addr_t MPIDU_Shm_abs_to_rel(void *a)
{
    MPIDU_Shm_rel-addr_t ret;
    ret.offset = (char *)a - (MPI_Aint)MPID_nem_asymm_base_addr;
    return ret;
}

/* The shadow head pointer makes this queue implementation quite a bit more
   performant.  Unfortunately, it also makes it a bit non-intuitive when read
   the code.  The following is an excerpt from "Design and Evaluation of Nemesis,  
   a Scalable, Low-Latency, Message-Passing Communication Subsystem" by
   D. Buntinas, G. Mercier, and W. Gropp that gives an explanation:

      A process must access both the head and tail of the queue when it is
      enqueuing an element on an empty queue or when it is dequeuing an element
      that is the last element in the queue. In these cases, if the head and
      tail were in the same cache line, only one L2 cache miss would be
      encountered. If the queue has more elements in it, however, then the
      enqueuer only needs to access the tail, and the dequeuer only needs to
      access the head. If the head and tail were in the same cache line, then
      there would be L2 misses encountered as a result of false sharing each
      time a process enqueues an element after another has been dequeued from
      the same queue, and vice versa. In this case it would be better if the
      head and tail were in separate cache lines.

      Our solution is to put the head and tail in the same cache line and have a
      shadow head pointer in a separate cache line. The shadow head is
      initialized to NULL. The dequeuer uses the shadow head in place of the
      real head except when the shadow head is NULL, meaning that the queue has
      become empty. If the shadow head is NULL when the dequeuer tries to
      dequeue, it checks the value of the real head. If the real head is not
      NULL, meaning that an element has been enqueued on the queue since the
      last time the queue became empty, the dequeuer initializes its shadow head
      to the value of the real head and sets the real head to NULL. In this way,
      only one L2 cache miss is encountered when enqueuing onto an empty queue
      or dequeuing from a queue with one element. And because the tail and
      shadow head are in separate cache lines, there are no L2 cache misses from
      false sharing. 

      We found that using a shadow head pointer reduced one-way latency by about
      200 ns on a dual 2 GHz Xeon node.
*/

/* All absolute and relative pointers point to the start of the enclosing element. */
typedef struct MPIDU_Queue_info_t {
    MPIDU_Shm_rel_addr_t head; /* holds the offset pointer, not the abs ptr */
    MPIDU_Shm_rel_addr_t tail; /* holds the offset pointer, not the abs ptr */
    char padding1[CACHELINE_SIZE-2*sizeof(MPIDU_Shm_rel_addr_t)];
    MPIDU_Shm_rel_addr_t  shadow_head; /* holds the offset pointer, not the abs ptr */
    char padding2[CACHELINE_SIZE-sizeof(MPIDU_Shm_rel_addr_t)];
} MPIDU_Queue_info_t;

/* Using this header struct even though it's just one element gives us the
   opportunity to vary the implementation more easily in the future without
   updating all callers. */
typedef struct MPIDU_Queue_element_hdr_t {
    MPIDU_Shm_rel_addr_t next; /* holds the offset pointer, not the abs ptr */
} MPIDU_Queue_element_hdr_t;


void MPIDU_Queue_init(MPIDU_Queue_info *qhead)
{
    MPIDU_SHM_SET_REL_NULL((qhead)->head);
    MPIDU_SHM_SET_REL_NULL((qhead)->tail);
    MPIDU_SHM_SET_REL_NULL((qhead)->shadow_head);
}

static inline int MPIDU_Queue_is_empty(MPIDU_Queue_info *qhead)
{
    int __ret = 0;
    if (MPIDU_SHM_IS_REL_NULL (qhead->shadow_head)) {
        if (MPIDU_SHM_IS_REL_NULL(qhead->head)) {
            __ret = 1;
        }
        else {
            qhead->shadow_head = qhead->head;
            MPIDU_SHM_SET_REL_NULL(qhead->head); /* reset it for next time */
        }
    }
    __ret;
}

/* This macro atomically enqueues an element (elt for short) into the queue
   indicated by qhead_ptr.  You need to pass several arguments:
     qhead_ptr - a pointer to a MPIDU_Queue_info_t structure to which the
                 element should be enqueued
     elt_ptr - a pointer to an element structure type that is to be enqueued
     elt_type - The base type of elt_ptr.  That is, if elt_ptr is a
                '(struct example_t *)' then elt_type is 'struct example_t'.
     elt_hdr_field - This is the member name of elt_type that is a
                     MPIDU_Queue_element_hdr_t.

    This queue implementation is loosely modeled after the linked lists used in
    the linux kernel.  You put the list header structure inside of the client
    structure, rather than the other way around.
   */
#define MPIDU_Queue_enqueue(qhead_ptr, elt_ptr, elt_type, elt_hdr_field) \
do {                                                                     \
    MPIDU_Shm_rel_addr_t r_prev;                                         \
    MPIDU_Shm_rel_addr_t r_elt = MPIDU_Shm_abs_to_rel(elt_ptr);          \
                                                                         \
    MPIDU_SHM_SET_REL_NULL((elt_ptr)->elt_hdr_field->next);              \
                                                                         \
    r_prev = MPIDU_SHM_SWAP_REL(&((qhead)->tail), r_elt);                \
                                                                         \
    if (MPIDU_SHM_IS_REL_NULL(r_prev)) {                                 \
        (qhead)->head = r_elt;                                           \
    }                                                                    \
    else {                                                               \
        elt_type *abs_prev = (elt_type *)MPIDU_Shm_rel_to_abs(r_prev)    \
        abs_prev->elt_hdr_field->next = r_elt;                           \
    }                                                                    \
} while (0)

/* This macro atomically dequeues an element (elt for short) from the queue
   indicated by qhead_ptr.  You need to pass several arguments:
     qhead_ptr - a pointer to a MPIDU_Queue_info_t structure to which the
                 element should be dequeued
     elt_ptr - a pointer to an element structure type that is to be dequeued
     elt_type - The base type of elt_ptr.  That is, if elt_ptr is a
                '(struct example_t *)' then elt_type is 'struct example_t'.
     elt_hdr_field - This is the member name of elt_type that is a
                     MPIDU_Queue_element_hdr_t.

    This queue implementation is loosely modeled after the linked lists used in
    the linux kernel.  You put the list header structure inside of the client
    structure, rather than the other way around.  */
#define MPIDU_Queue_dequeue(qhead_ptr, elt_ptr_ptr, elt_type, elt_hdr_field) \
do {                                                                         \
    register elt_type *_e;                                                   \
    register MPIDU_Shm_rel_addr_t _r_e;                                      \
                                                                             \
    _r_e = (qhead)->shadow_head;                                             \
    _e = MPIDU_Shm_rel_to_abs(_r_e);                                         \
                                                                             \
    if (!MPIDU_SHM_IS_REL_NULL(_e->elt_hdr_field->next)) {                   \
        (qhead)->shadow_head = _e->elt_hdr_field->next;                      \
    }                                                                        \
    else {                                                                   \
        MPIDU_Shm_rel_addr_t old_tail;                                       \
                                                                             \
        MPIDU_SHM_SET_REL_NULL((qhead)->shadow_head);                        \
                                                                             \
        old_tail = MPIDU_SHM_CAS_REL_NULL(&((qhead)->tail), _r_e);           \
                                                                             \
        if (!MPIDU_SHM_REL_ARE_EQUAL(old_tail, _r_e)) {                      \
            while (MPIDU_SHM_IS_REL_NULL(_e->elt_hdr_field->next)) {         \
                SKIP;                                                        \
            }                                                                \
            (qhead)->shadow_head = _e->elt_hdr_field->next;                  \
        }                                                                    \
    }                                                                        \
    MPIDU_SHM_SET_REL_NULL(_e->elt_hdr_field->next);                         \
    *(elt_ptr_ptr) = _e;                                                     \
} while (0)                                             


#endif /* MPIDU_QUEUE_H_INCLUDED */
