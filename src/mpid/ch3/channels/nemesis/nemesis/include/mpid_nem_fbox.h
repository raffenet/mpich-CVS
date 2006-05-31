/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_FBOX_H
#define MPID_NEM_FBOX_H

typedef struct MPID_nem_fboxq_elem
{
  int usage;
  struct MPID_nem_fboxq_elem *prev;
  struct MPID_nem_fboxq_elem *next;
  int grank;
  MPID_nem_fbox_mpich2_t *fbox;
} MPID_nem_fboxq_elem_t ;

extern MPID_nem_fboxq_elem_t *fboxq_head;
extern MPID_nem_fboxq_elem_t *fboxq_tail;
extern MPID_nem_fboxq_elem_t *fboxq_elem_list;
extern MPID_nem_fboxq_elem_t *fboxq_elem_list_last;
extern MPID_nem_fboxq_elem_t *curr_fboxq_elem;
extern MPID_nem_fboxq_elem_t *curr_fbox_all_poll;
extern unsigned short        *send_seqno;
extern unsigned short        *recv_seqno;
int    MPID_nem_mpich2_dequeue_fastbox (int local_rank);
int    MPID_nem_mpich2_enqueue_fastbox (int local_rank);

#if 0 /* papi timing stuff added */
#define poll_fboxes(_cell, do_found) do {								\
    MPID_nem_fboxq_elem_t *orig_fboxq_elem;									\
													\
    if (fboxq_head != NULL)										\
    {													\
	orig_fboxq_elem = curr_fboxq_elem;								\
	do												\
	{												\
	    int value;											\
	    MPID_nem_fbox_mpich2_t *fbox;										\
													\
	    fbox = curr_fboxq_elem->fbox;								\
													\
	    DO_PAPI2 (PAPI_reset (PAPI_EventSet));							\
	    value = fbox->flag.value;									\
	    if (value == 1)										\
		DO_PAPI2 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues9));				\
										       		\
	    if (value == 1 && fbox->cell.pkt.mpich2.seqno == recv_seqno[curr_fboxq_elem->grank])	\
	    {												\
		++recv_seqno[curr_fboxq_elem->grank];							\
		*(_cell) = &fbox->cell;									\
		do_found;										\
	    }												\
	    curr_fboxq_elem = curr_fboxq_elem->next;							\
	    if (curr_fboxq_elem == NULL)								\
		curr_fboxq_elem = fboxq_head;								\
	}												\
	while (curr_fboxq_elem != orig_fboxq_elem);							\
    }													\
    *(_cell) = NULL;											\
} while (0)
#else
#define poll_fboxes(_cell, do_found) do {									\
    MPID_nem_fboxq_elem_t *orig_fboxq_elem;										\
														\
    if (fboxq_head != NULL)											\
    {														\
	orig_fboxq_elem = curr_fboxq_elem;									\
	do													\
	{													\
	    MPID_nem_fbox_mpich2_t *fbox;											\
														\
	    fbox = curr_fboxq_elem->fbox;									\
	    if (fbox->flag.value == 1 && fbox->cell.pkt.mpich2.seqno == recv_seqno[curr_fboxq_elem->grank])	\
	    {													\
                ++recv_seqno[curr_fboxq_elem->grank];								\
		*(_cell) = &fbox->cell;										\
		do_found;											\
	    }													\
	    curr_fboxq_elem = curr_fboxq_elem->next;								\
	    if (curr_fboxq_elem == NULL)									\
		curr_fboxq_elem = fboxq_head;									\
	}													\
	while (curr_fboxq_elem != orig_fboxq_elem);								\
    }														\
    *(_cell) = NULL;												\
} while (0)
#endif
     
#define poll_all_fboxes(_cell, do_found) do {										\
    MPID_nem_fbox_mpich2_t *fbox;													\
															\
    fbox = curr_fbox_all_poll->fbox;											\
    if (fbox && fbox->flag.value == 1 && fbox->cell.pkt.mpich2.seqno == recv_seqno[curr_fbox_all_poll->grank])	\
    {															\
	++recv_seqno[curr_fbox_all_poll->grank];									\
	*(_cell) = &fbox->cell;												\
	do_found;													\
    }															\
    ++curr_fbox_all_poll;												\
    if (curr_fbox_all_poll > fboxq_elem_list_last)									\
	curr_fbox_all_poll = fboxq_elem_list;										\
} while(0)

#endif //MPID_NEM_FBOX_H


