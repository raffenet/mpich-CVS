/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the utility file for comm that contains the basic comm items
   and storage management */
#ifndef MPID_DATATYPE_PREALLOC 
#define MPID_DATATYPE_PREALLOC 8
#endif

/* Preallocated comm objects */
MPID_Datatype MPID_Datatype_direct[MPID_DATATYPE_PREALLOC];
static int initialized = 0;

static MPIU_Handle_common *avail = 0;

/* Extension objects */
static MPID_Datatype *(*MPID_Datatype_indirect)[] = 0;
static int MPID_Datatype_indirect_size = 0;

/* 
 * Get an pointer to dynamically allocated storage for Datatype objects.
 * This has an MPID prefix to simplify the "get object" routines
 */
MPID_Datatype *MPID_Datatype_Get_ptr_indirect( int handle )
{
    int block_num, index_num;

    /* Check for a valid handle type */
    if (HANDLE_GET_MPI_KIND(handle) != MPID_DATATYPE) {
	return 0;
    }

    /* Find the block */
    block_num = HANDLE_BLOCK(handle);
    if (block_num >= MPID_Datatype_indirect_size) {
	return 0;
    }
    
    /* Find the entry */
    index_num = HANDLE_BLOCK_INDEX(handle);
    return &(*MPID_Datatype_indirect)[block_num][index_num];
}

/* This routine is called by finalize when MPI exits */
static int MPID_Datatype_finalize( void *extra )
{
    (void)MPIU_Handle_free( (void *(*)[])MPID_Datatype_indirect, 
			    MPID_Datatype_indirect_size );
    /* This does *not* remove any Datatype objects that the user created 
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
MPID_Datatype *MPID_Datatype_create( void )
{
    MPID_Datatype *ptr;

    /* Lock if necessary */
    MPID_Allocation_lock();

    if (avail) {
	ptr     = (MPID_Datatype *)avail;
	avail   = avail->next;
	/* Unlock */
	MPID_Allocation_unlock();
	/* printf ("returning comm %x\n", ptr->id ); */
	return ptr;
    }

    if (!initialized) {
	/* Setup the first block.  This is done here so that short MPI
	   jobs do not need to include any of the Datatype code if no
	   Derived-Datatype-using routines are used */
	/* Tell finalize to free up any memory that we allocate */
	MPIR_Add_finalize( MPID_Datatype_finalize, 0 );

	initialized = 1;
	ptr   = MPIU_Handle_direct_init( MPID_Datatype_direct, MPID_DATATYPE_PREALLOC,
					 sizeof(MPID_Datatype), MPID_DATATYPE );
	if (ptr)
	    avail = ((MPIU_Handle_common *)ptr)->next;
	/* unlock */
	MPID_Allocation_unlock();
	return ptr;
    }

    /* Must create new storage for dynamically allocated objects */
    ptr = MPIU_Handle_indirect_init( (void *(**)[])&MPID_Datatype_indirect, 
				     &MPID_Datatype_indirect_size, 
				     HANDLE_BLOCK_SIZE, 
				     HANDLE_BLOCK_INDEX_SIZE,
				     sizeof(MPID_Datatype), MPID_DATATYPE );
    if (ptr)
	avail = ((MPIU_Handle_common *)ptr)->next;

    /* Unlock */
    MPID_Allocation_unlock();
    return ptr;
}   

/* Free an comm structure */
void MPID_Datatype_free( MPID_Datatype *datatype_ptr )
{
    /* Lock because updating avail list */
    MPID_Allocation_lock();

    /* Return comm to the avail list */
    ((MPIU_Handle_common *)datatype_ptr)->next = avail;
    avail          = (MPIU_Handle_common *)datatype_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}

/* return just this entry to the avail list.  Should this be called remove 
   instead of delete since all it does is deallocate that value? */
void MPID_Datatype_destroy( MPID_Datatype *datatype_ptr )
{
    /* Lock */
    MPID_Allocation_lock();
    ((MPIU_Handle_common *)datatype_ptr)->next = avail;
    avail          = (MPIU_Handle_common *)datatype_ptr;
    /* Unlock */
    MPID_Allocation_unlock();
}

/* 
 * This routine computes the extent of a datatype.  
 * This routine handles not only the various upper bound and lower bound
 * markers but also the alignment rules set by the environment (the PAD).
 */
/* *** NOT DONE *** */
#ifdef FOO
MPI_Aint MPIR_Type_compute_extent( MPID_Datatype *datatype_ptr )
{
    /* Compute the \mpids{MPI_Datatype}{ub} */
    If a sticky ub exists for the old datatype (datatypes for struct) {
	use the \mpids{MPI_Datatype}{sticky_ub} and set the sticky ub flag
        (\mpids{MPI_Datatype}{MPID_TYPE_STICKY_UB}).
    }
    else {
        use the \mpids{MPI_Datatype}{true_ub}
    }
   /* Similar for the \mpids{MPI_Datatype}{lb},
   \mpids{MPI_Datatype}{sticky_lb},
   \mpids{MPI_Datatype}{MPID_TYPE_STICKY_LB}   and
   \mpids{MPI_Datatype}{true_lb}. */

   /* Determine PAD from alignment rules */
   datatype_ptr->extent = datatype_ptr->ub - datatype_ptr->lb + PAD;

   /* Similar for \mpids{MPI_Datatype}{true_extent} */
									       }
#endif
