/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the utility file for info that contains the basic info items
   and storage management */
#ifndef MPID_INFO_PREALLOC 
#define MPID_INFO_PREALLOC 64
#endif

/* Preallocated info objects */
MPID_Info MPID_Info_direct[MPID_INFO_PREALLOC];
static int initialized = 0;

/* Next available info object */
static MPID_Info *avail = 0;

/* Extension objects */
static MPID_Info (*MPID_Info_indirect)[] = 0;
static int MPID_Info_indirect_size = 0;

/* 
 * Get an pointer to dynamically allocated storage for Info objects.
 *
 */
MPID_Info *MPIU_Info_Get_ptr_indirect( int handle )
{
    /* Check for a valid handle type */
    if (HANDLE_KIND(handle) != MPID_INFO) {
	return 0;
    }

    /* Find the block */
    block_num = HANDLE_BLOCK(handle);
    if (block_num >= MPID_Info_indirect_size) {
	return 0;
    }
    
    /* Find the entry */
    index_num = HANDLE_BLOCK_INDEX(handle);
    return &MPID_Info_indirect[block_num][index_num];
}

/* This routine is called by finalize when MPI exits */
static int MPIU_Info_finalize( void *extra )
{
    /* Remove any allocated storage */
    for (i=0; i<MPID_Info_indirect_size; i++) {
	MPIU_Free( MPID_Info_indirect[i] );
    }
    if (MPID_Info_indirect) {
	MPIU_Free( MPID_Info_indirect );
    }
    /* This does *not* remove any Info objects that the user created 
       and then did not destroy */
    return 0;
}

/*
  Create and return a pointer to an info object.  Returns null if there is 
  an error such as out-of-memory.  Does not allocate space for the
  key or value.

  Also note that this routine must be thread-safe.  The most robust version 
  of this would use atomic instructions to manage memory, so that there
  would never be any possibility of a process exiting while holding a lock.
  However, for simplicity, this uses a common per-process lock.
 */
MPID_Info *MPIU_Info_create( void )
{
    MPID_Info *ptr;
    int       i;

    /* Lock if necessary */
    MPID_Allocation_lock();

    if (avail) {
	ptr     = avail;
	avail   = avail->next;
	/* Unlock */
	MPID_Allocation_unlock();
	return ptr;
    }

    if (!initialized) {
	/* Setup the first block.  This is done here so that short MPI
	   jobs do not need to include any of the Info code if no
	   Info-using routines are used */
	/* Tell finalize to free up any memory that we allocate */
	MPIR_Add_finalize( MPIU_Info_finalize, 0 );

	initialized = 1;
	for (i=0; i<MPID_INFO_PREALLOC; i++) {
	    MPID_Info_direct[i].next = MPID_Info_direct + i + 1;
	    MPID_Info_direct[i].id   = (CONSTRUCT_DIRECT << 30) |
	    (MPID_INFO << 27) | i;
	}
	MPID_Info_direct[i-1].next = 0;
	avail = &MPID_Info_direct[1];
	ptr   = &MPID_Info_direct[0];
	/* unlock */
	MPID_Allocation_unlock();
	return ptr;
    }

    /* Must create new storage for dynamically allocated objects */
    /* Create the table */
    if (!MPID_Info_indirect) {
	MPID_Info_indirect = (MPID_Info **)MPIU_Calloc( HANDLE_BLOCK_SIZE,
							 sizeof(MPID_Info*) );
	if (!MPID_Info_indirect) { 
	    /* Unlock */
	    MPID_Allocation_unlock();
	    return 0;
	}
    }
    
    /* Create the next block */
    block_ptr = (MPID_Info*)MPIU_Calloc( HANDLE_BLOCK_INDEX_SIZE,
					  sizeof(MPID_Info) );
    if (!block_ptr) { 
	/* unlock */
	return 0;
    }
    for (i=0; i<HANDLE_BLOCK_INDEX_SIZE; i++) {
	block_ptr[i].next = block_ptr + i + 1;
	block_ptr[i].id   = (CONSTRUCT_INDIRECT << 30) |
	    (MPID_INFO << 27) | (MPID_Info_indirect_size << 16) | i;
    }
    block_ptr[i-1].next = 0;
    ptr   = &block_ptr[0];
    avail = &block_ptr[1];
    MPID_Info_indirect[MPID_Info_indirect_size++] = block_ptr;

    /* Unlock */
    MPID_Allocation_unlock();
    return ptr;
}   

/* Free an info structure */
void MPIU_Info_free( MPID_Info *info_ptr )
{
    MPID_Info *curr_ptr, *next_ptr, *last_ptr;

    curr_ptr = info_ptr->next;
    last_ptr = info_ptr;

    /* First, free the string storage */
    while (curr_ptr) {
	next_ptr = curr_ptr->next;
	MPIU_Free(curr_ptr->key);
	MPIU_Free(curr_ptr->value);
	last_ptr = curr_ptr;
	curr_ptr = next_ptr;
    }

    /* Lock because updating avail list */
    MPID_Allocation_lock();

    /* Return info to the avail list */
    last_ptr->next = avail;
    avail          = info_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}

/* return just this entry to the avail list.  Should this be called remove 
   instead of delete since all it does is deallocate that value? */
void MPIU_Info_destroy( MPID_Info *info_ptr )
{
    /* Lock */
    MPID_Allocation_lock();
    info_ptr->next = avail;
    avail          = info_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
 }

