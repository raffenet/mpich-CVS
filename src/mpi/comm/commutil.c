/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */
#ifndef MPID_COMM_PREALLOC 
#define MPID_COMM_PREALLOC 8
#endif

/* Preallocated comm objects */
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC];
static int initialized = 0;

/* Next available comm object */
typedef struct _MPID_Comm_list { 
    struct _MPID_Comm_list *next; } MPID_Comm_list;
static MPID_Comm_list *avail = 0;

/* Extension objects */
static MPID_Comm *(*MPID_Comm_indirect)[] = 0;
static int MPID_Comm_indirect_size = 0;

/* 
 * Get an pointer to dynamically allocated storage for Comm objects.
 * This has an MPID prefix to simplify the "get object" routines
 */
MPID_Comm *MPID_Comm_Get_ptr_indirect( int handle )
{
    int block_num, index_num;

    /* Check for a valid handle type */
    if (HANDLE_KIND(handle) != MPID_COMM) {
	return 0;
    }

    /* Find the block */
    block_num = HANDLE_BLOCK(handle);
    if (block_num >= MPID_Comm_indirect_size) {
	return 0;
    }
    
    /* Find the entry */
    index_num = HANDLE_BLOCK_INDEX(handle);
    return &(*MPID_Comm_indirect)[block_num][index_num];
}

/* This routine is called by finalize when MPI exits */
static int MPIU_Comm_finalize( void *extra )
{
    int i;
    
    /* Remove any allocated storage */
    for (i=0; i<MPID_Comm_indirect_size; i++) {
	MPIU_Free( (*MPID_Comm_indirect)[i] );
    }
    if (MPID_Comm_indirect) {
	MPIU_Free( MPID_Comm_indirect );
    }
    /* This does *not* remove any Comm objects that the user created 
       and then did not destroy */
    return 0;
}

/*
  Create and return a pointer to an comm object.  Returns null if there is 
  an error such as out-of-memory.  Does not allocate space for the
  key or value.

  Also note that this routine must be thread-safe.  The most robust version 
  of this would use atomic instructions to manage memory, so that there
  would never be any possibility of a process exiting while holding a lock.
  However, for simplicity, this uses a common per-process lock.
 */
MPID_Comm *MPIU_Comm_create( void )
{
    MPID_Comm *ptr, *block_ptr;
    int       i;

    /* Lock if necessary */
    MPID_Allocation_lock();

    if (avail) {
	ptr     = (MPID_Comm*)avail;
	avail   = avail->next;
	/* Unlock */
	MPID_Allocation_unlock();
	/* printf ("returning comm %x\n", ptr->id ); */
	return ptr;
    }

    if (!initialized) {
	/* Setup the first block.  This is done here so that short MPI
	   jobs do not need to include any of the Comm code if no
	   Comm-using routines are used */
	/* Tell finalize to free up any memory that we allocate */
	MPIR_Add_finalize( MPIU_Comm_finalize, 0 );

	initialized = 1;
	for (i=0; i<MPID_COMM_PREALLOC; i++) {
	    ((MPID_Comm_list*)&MPID_Comm_direct[i])->next = (MPID_Comm_list*)(MPID_Comm_direct + i + 1);
	    MPID_Comm_direct[i].id = (CONSTRUCT_DIRECT << 30) |
	    (MPID_COMM << 27) | i;
	}
	((MPID_Comm_list *)&MPID_Comm_direct[i-1])->next = 0;
	avail = (MPID_Comm_list *)&MPID_Comm_direct[1];
	ptr   = &MPID_Comm_direct[0];
	/* unlock */
	MPID_Allocation_unlock();
	return ptr;
    }

    /* Must create new storage for dynamically allocated objects */
    /* Create the table */
    if (!MPID_Comm_indirect) {
	/* printf( "Creating indirect table\n" ); */
	MPID_Comm_indirect = (MPID_Comm *(*)[])MPIU_Calloc( HANDLE_BLOCK_SIZE,
							 sizeof(MPID_Comm*) );
	if (!MPID_Comm_indirect) { 
	    /* Unlock */
	    MPID_Allocation_unlock();
	    return 0;
	}
    }

    if (MPID_Comm_indirect_size >= HANDLE_BLOCK_SIZE-1) {
	/* unlock */
	MPID_Allocation_unlock();
	return 0;
    }
    
    /* Create the next block */
    /* printf( "Adding indirect block %d\n", MPID_Comm_indirect_size ); */
    block_ptr = (MPID_Comm*)MPIU_Calloc( HANDLE_BLOCK_INDEX_SIZE,
					  sizeof(MPID_Comm) );
    if (!block_ptr) { 
	/* unlock */
	MPID_Allocation_unlock();
	return 0;
    }
    for (i=0; i<HANDLE_BLOCK_INDEX_SIZE; i++) {
	((MPID_Comm_list *)&block_ptr[i])->next = (MPID_Comm_list*)(block_ptr + i + 1);
	block_ptr[i].id   = (CONSTRUCT_INDIRECT << 30) |
	    (MPID_COMM << 27) | (MPID_Comm_indirect_size << 16) | i;
    }
    ((MPID_Comm_list*)&block_ptr[i-1])->next = 0;
    ptr   = &block_ptr[0];
    /* We're here because avail is null, so there is no need to set 
       the last block ptr to avail */
    avail = (MPID_Comm_list *)&block_ptr[1];
    /* printf( "loc of update is %x\n", &(*MPID_Comm_indirect)[MPID_Comm_indirect_size] ); */
    (*MPID_Comm_indirect)[MPID_Comm_indirect_size++] = block_ptr;

    /* Unlock */
    MPID_Allocation_unlock();
    return ptr;
}   

/* Free an comm structure */
void MPIU_Comm_free( MPID_Comm *comm_ptr )
{
    /* Lock because updating avail list */
    MPID_Allocation_lock();

    /* Return comm to the avail list */
    ((MPID_Comm_list *)comm_ptr)->next = avail;
    avail          = (MPID_Comm_list *)comm_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}

/* return just this entry to the avail list.  Should this be called remove 
   instead of delete since all it does is deallocate that value? */
void MPIU_Comm_destroy( MPID_Comm *comm_ptr )
{
    /* Lock */
    MPID_Allocation_lock();
    ((MPID_Comm_list *)comm_ptr)->next = avail;
    avail          = (MPID_Comm_list *)comm_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}
