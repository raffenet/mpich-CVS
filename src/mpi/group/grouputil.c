/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "group.h"

#ifndef MPID_GROUP_PREALLOC 
#define MPID_GROUP_PREALLOC 8
#endif

/* Preallocated group objects */
MPID_Group MPID_Group_builtin[MPID_GROUP_N_BUILTIN] = {
    { MPI_GROUP_EMPTY, 1, 0, MPI_UNDEFINED, -1, 0, } };
MPID_Group MPID_Group_direct[MPID_GROUP_PREALLOC];
MPIU_Object_alloc_t MPID_Group_mem = { 0, 0, 0, 0, MPID_GROUP, 
				      sizeof(MPID_Group), MPID_Group_direct,
				       MPID_GROUP_PREALLOC};

/* 
 * Allocate a new group and the group lrank to lpid array.  Does *not* 
 * initialize any arrays, but does set the reference count.
 */
int MPIR_Group_create( int nproc, MPID_Group **new_group_ptr )
{
    int mpi_errno = MPI_SUCCESS;

    *new_group_ptr = (MPID_Group *)MPIU_Handle_obj_alloc( &MPID_Group_mem );
    if (!*new_group_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    (*new_group_ptr)->ref_count = 1;
    (*new_group_ptr)->lrank_to_lpid = 
	(MPID_Group_pmap_t *)MPIU_Malloc( nproc * sizeof(MPID_Group_pmap_t) );
    if (!(*new_group_ptr)->lrank_to_lpid) {
	MPIU_Handle_obj_free( &MPID_Group_mem, *new_group_ptr );
	*new_group_ptr = NULL;
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    (*new_group_ptr)->size = nproc;
    return mpi_errno;
}
/*
 * return value is the first index in the list
 *
 * This sorts an lpid array by lpid value, using a simple merge sort
 * algorithm.
 *
 */
static int MPIR_Mergesort_lpidarray( MPID_Group_pmap_t maparray[], int n )
{
    int idx1, idx2, first_idx, cur_idx, next_lpid, idx2_offset;

    if (n == 2) {
	if (maparray[0].lpid > maparray[1].lpid) {
	    first_idx = 1;
	    maparray[0].next_lpid = -1;
	    maparray[1].next_lpid = 0;
	}
	else {
	    first_idx = 0;
	    maparray[0].next_lpid = 1;
	    maparray[1].next_lpid = -1;
	}
	return first_idx;
    }
    if (n == 1) {
	maparray[0].next_lpid = -1;
	return 0;
    }
    if (n == 0) 
	return -1;

    /* Sort each half */
    idx2_offset = n/2;
    idx1 = MPIR_Mergesort_lpidarray( maparray, n/2 );
    idx2 = MPIR_Mergesort_lpidarray( maparray + idx2_offset, n - n/2 ) + idx2_offset;
    /* merge the results */
    /* There are three lists:
       first_idx - points to the HEAD of the sorted, merged list
       cur_idx - points to the LAST element of the sorted, merged list
       idx1    - points to the HEAD of one sorted list
       idx2    - points to the HEAD of the other sorted list
       
       We first identify the head element of the sorted list.  We then 
       take elements from the remaining lists.  When one list is empty,
       we add the other list to the end of sorted list. 

       The last wrinkle is that the next_lpid fields in maparray[idx2]
       are relative to n/2, not 0 (that is, a next_lpid of 1 is
       really 1 + n/2, relative to the beginning of maparray).
    */
    /* Find the head element */
    if (maparray[idx1].lpid > maparray[idx2].lpid) {
	first_idx = idx2;
	idx2      = maparray[idx2].next_lpid + idx2_offset;
    }
    else {
	first_idx = idx1;
	idx1      = maparray[idx1].next_lpid;
    }
    
    /* Merge the lists until one is empty */
    cur_idx = first_idx;
    while ( idx1 >= 0 && idx2 >= 0) {
	if (maparray[idx1].lpid > maparray[idx2].lpid) {
	    next_lpid			= maparray[idx2].next_lpid;
	    if (next_lpid >= 0) next_lpid += idx2_offset;
	    maparray[cur_idx].next_lpid	= idx2;
	    cur_idx			= idx2;
	    idx2			= next_lpid;
	}
	else {
	    next_lpid			= maparray[idx1].next_lpid;
	    maparray[cur_idx].next_lpid	= idx1;
	    cur_idx			= idx1;
	    idx1			= next_lpid;
	}
    }
    /* Add whichever list remains */
    if (idx1 >= 0) {
	maparray[cur_idx].next_lpid = idx1;
    }
    else {
	maparray[cur_idx].next_lpid = idx2;
	/* Convert the rest of these next_lpid values to be 
	   relative to the beginning of maparray */
	while (idx2 >= 0) {
	    next_lpid = maparray[idx2].next_lpid;
	    if (next_lpid >= 0) {
		next_lpid += idx2_offset;
		maparray[idx2].next_lpid = next_lpid;
	    }
	    idx2 = next_lpid;
	}
    }

    return first_idx;
}

/* 
 * Create a list of the lpids, in lpid order.
 */
void MPIR_Group_setup_lpid_list( MPID_Group *group_ptr )
{
    /* Lock around the data structure updates in case another thread
       decides to update the same group.  Note that this is needed only
       for MPI_THREAD_MULTIPLE */
    MPID_Common_thread_lock();
    {
	if (group_ptr->idx_of_first_lpid == -1) {
	    group_ptr->idx_of_first_lpid = 
		MPIR_Mergesort_lpidarray( group_ptr->lrank_to_lpid, 
					  group_ptr->size );
	}
    }
    MPID_Common_thread_unlock();
    return;
}

void MPIR_Group_setup_lpid_pairs( MPID_Group *group_ptr1, 
				  MPID_Group *group_ptr2 )
{
    /* If the lpid list hasn't been created, do it now */
    if (group_ptr1->idx_of_first_lpid < 0) { 
	MPIR_Group_setup_lpid_list( group_ptr1 ); 
    }
    if (group_ptr2->idx_of_first_lpid < 0) { 
	MPIR_Group_setup_lpid_list( group_ptr2 ); 
    }
}

#ifdef HAVE_ERROR_CHECKING
/* 
 * The following routines are needed only for error checking
 */

/*
 * This routine is for error checking for a valid ranks array, used
 * by Group_incl and Group_excl
 */
int MPIR_Group_check_valid_ranks( MPID_Group *group_ptr, int ranks[], int n )
{
    int mpi_errno = MPI_SUCCESS, i;

    /* Thread lock in case any other thread wants to use the group
       data structure.  Needed only for MPI_THREAD_MULTIPLE */
    MPID_Common_thread_lock();
    {
	for (i=0; i<group_ptr->size; i++) {
	    group_ptr->lrank_to_lpid[i].flag = 0;
	}
	for (i=0; i<n; i++) {
	    if (ranks[i] < 0 ||
		ranks[i] >= group_ptr->size) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_RANK,
				  "**rankarray", "**rankarray %d %d %d",
				  i, ranks[i], group_ptr->size );
	    }
	    if (group_ptr->lrank_to_lpid[ranks[i]].flag) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_RANK,
				"**rankdup", "**rankdup %d %d %d",
				  i, ranks[i], 
				  group_ptr->lrank_to_lpid[ranks[i]].flag-1);
	    }
	    group_ptr->lrank_to_lpid[ranks[i]].flag = i+1;
	}
    }
    MPID_Common_thread_unlock();

    return mpi_errno;
}

int MPIR_Group_check_valid_ranges( MPID_Group *group_ptr, 
				   int ranges[][3], int n )
{
    int i, j, size, first, last, stride, mpi_errno = MPI_SUCCESS;

    /* Lock in case another thread is accessing the group 
       data structures */
    MPID_Common_thread_lock();
    size = group_ptr->size;
    
    /* First, clear the flag */
    for (i=0; i<size; i++) {
	group_ptr->lrank_to_lpid[i].flag = 0;
    }
    for (i=0; i<n; i++) {
	first = ranges[i][0]; last = ranges[i][1]; 
	stride = ranges[i][2];
	if (first < 0 || first >= size) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG,
					      "**rangeinvalid", 
					      "**rangestartinvalid %d %d %d", 
					      i, first, size );
	    break;
	}
	if (last < 0 || last >= size) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG,
					      "**rangeinvalid", 
					      "**rangeendinvalid %d %d %d", 
					      i, last, size );
	    break;
	}
	if (stride != 0) {
	    if ( (stride > 0 && first > last) ||
		 (stride < 0 && first < last) ) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
						  "**stride", "**stride %d %d %d", 
						  first, last, stride );
		break;
	    }
	}
	else {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
					      "**stridezero", 0 );
	    break;
	}
	/* range is valid.  Mark flags */
	if (stride > 0) {
	    for (j=first; j<=last; j+=stride) {
		if (group_ptr->lrank_to_lpid[j].flag) {
		    mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG,
						      "**rangedup", 0 );
		    break;
		}
		else
		    group_ptr->lrank_to_lpid[i].flag = 1;
	    }
	}
	else {
	    for (j=first; j>=last; j+=stride) {
		if (group_ptr->lrank_to_lpid[j].flag) {
		    mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG,
						      "**rangedup", 0 );
		    break;
		}
		else
		    group_ptr->lrank_to_lpid[i].flag = 1;
	    }
	}
	if (mpi_errno) break;
    }
    MPID_Common_thread_unlock();

    return mpi_errno;
}
#endif  /* HAVE_ERROR_CHECKING */
