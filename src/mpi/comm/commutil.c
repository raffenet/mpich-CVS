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

static MPIU_Handle_common *avail = 0;

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
    (void)MPIU_Handle_free( (void *(*)[])MPID_Comm_indirect, 
			    MPID_Comm_indirect_size );
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
    MPID_Comm *ptr;

    /* Lock if necessary */
    MPID_Allocation_lock();

    if (avail) {
	ptr     = (MPID_Comm *)avail;
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
	ptr   = MPIU_Handle_direct_init( MPID_Comm_direct, MPID_COMM_PREALLOC,
					 sizeof(MPID_Comm), MPID_COMM );
	if (ptr)
	    avail = ((MPIU_Handle_common *)ptr)->next;
	/* unlock */
	MPID_Allocation_unlock();
	return ptr;
    }

    /* Must create new storage for dynamically allocated objects */
    ptr = MPIU_Handle_indirect_init( (void *(**)[])&MPID_Comm_indirect, 
				     &MPID_Comm_indirect_size, 
				     HANDLE_BLOCK_SIZE, 
				     HANDLE_BLOCK_INDEX_SIZE,
				     sizeof(MPID_Comm), MPID_COMM );
    if (ptr)
	avail = ((MPIU_Handle_common *)ptr)->next;

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
    ((MPIU_Handle_common *)comm_ptr)->next = avail;
    avail          = (MPIU_Handle_common *)comm_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}

/* return just this entry to the avail list.  Should this be called remove 
   instead of delete since all it does is deallocate that value? */
void MPIU_Comm_destroy( MPID_Comm *comm_ptr )
{
    /* Lock */
    MPID_Allocation_lock();
    ((MPIU_Handle_common *)comm_ptr)->next = avail;
    avail          = (MPIU_Handle_common *)comm_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}
