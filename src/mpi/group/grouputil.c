/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#ifndef MPID_GROUP_PREALLOC 
#define MPID_GROUP_PREALLOC 8
#endif

/* Preallocated group objects */
MPID_Group MPID_Group_builtin[MPID_GROUP_N_BUILTIN];
MPID_Group MPID_Group_direct[MPID_GROUP_PREALLOC];
MPIU_Object_alloc_t MPID_Group_mem = { 0, 0, 0, 0, MPID_GROUP, 
				      sizeof(MPID_Group), MPID_Group_direct,
				       MPID_GROUP_PREALLOC};

/*
 * return value is the first index in the list
 */
int MPIR_Mergesort_lpidarray( MPID_Group_pmap_t maparray[], int n )
{
    int idx1, idx2, first_idx, cur_idx;

    if (n == 1) {
	maparray[0].next_lpid = -1;
	return 0;
    }

    /* Sort each half */
    idx1 = MPIR_Mergesort_lpidarray( maparray, n/2 );
    idx2 = MPIR_Mergesort_lpidarray( maparray + n/2 + 1, n - n/2 ) + n/2 + 1;

    /* merge the results */
    first_idx = idx1;
    if (maparray[idx1].lpid > maparray[idx2].lpid) first_idx = idx2;
    
    cur_idx = first_idx;
    while ( idx1 >= 0 && idx2 >= 0) {
	if (maparray[idx1].lpid > maparray[idx2].lpid) {
	    maparray[cur_idx].next_lpid = idx2;
	    cur_idx = idx2;
	    idx2 = maparray[idx2].next_lpid;
	}
	else {
	    maparray[cur_idx].next_lpid = idx1;
	    cur_idx = idx1;
	    idx1 = maparray[idx1].next_lpid;
	}
    }

    return first_idx;
}

/* 
 * Create a list of the lpids, in lpid order
 */
void MPIR_Group_setup_lpid_list( MPID_Group *group_ptr )
{
    int idx;
    
    group_ptr->idx_of_first_lpid = 
	MPIR_Mergesort_lpidarray( group_ptr->lrank_to_lpid, group_ptr->size );

    return;
}
