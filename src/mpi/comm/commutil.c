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
MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN];
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC];
MPIU_Object_alloc_t MPID_Comm_mem = { 0, 0, 0, 0, MPID_COMM, 
				      sizeof(MPID_Comm), MPID_Comm_direct,
                                      MPID_COMM_PREALLOC};

/* Create a new communicator with a context.  
   Do *not* initialize the other fields except for the reference count.
   See MPIR_Comm_copy for a function to produce a copy of part of a
   communicator 
 */
int MPIR_Comm_create( MPID_Comm *oldcomm_ptr, MPID_Comm **newcomm_ptr )
{   
    int mpi_errno, new_context_id;

    *newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!*newcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    (*newcomm_ptr)->ref_count = 1;

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id */
    (*newcomm_ptr)->context_id = new_context_id = 
	MPIR_Get_contextid( oldcomm_ptr->handle );
    if (new_context_id == 0) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**toomanycomm", 0 );
	return mpi_errno;
    }
    return 0;
}

/*
 * Here is the routine to find a new context id.  The algorithm is discussed 
 * in detail in the mpich2 coding document.  There are versions for
 * single threaded and multithreaded MPI (only single threaded implemented
 * so far).
 * 
 * These assume that int is 32 bits; they should use uint_32 instead, 
 * and an MPI_UINT32 type.
 */
#if MPID_MAX_THREAD_LEVEL <= MPI_THREAD_FUNNELED
#define MAX_CONTEXT_MASK 32
static unsigned int context_mask[MAX_CONTEXT_MASK];
static int initialize_context_mask = 1;

int MPIR_Get_contextid( MPI_Comm comm )
{
    int i, j, context_id = 0;
    unsigned int mask;
    unsigned int local_mask[MAX_CONTEXT_MASK];

    if (initialize_context_mask) {
	for (i=1; i<MAX_CONTEXT_MASK; i++) {
	    context_mask[i] = 0xFFFFFFFF;
	}
	/* the first two values are already used */
	context_mask[0] = 0x3FFFFFFF; 
	initialize_context_mask = 0;
    }
    memcpy( local_mask, context_mask, MAX_CONTEXT_MASK * sizeof(int) );
    NMPI_Allreduce( MPI_IN_PLACE, local_mask, MAX_CONTEXT_MASK, MPI_INT, 
		    MPI_BAND, comm );
    
    for (i=0; i<MAX_CONTEXT_MASK; i++) {
	if (local_mask[i]) {
	    /* There is a bit set in this word */
	    mask = 0x80000000;
	    /* This is a simple sequential search.  */
	    for (j=0; j<32; j++) {
		if (mask & local_mask[i]) {
		    /* Found the leading set bit */
		    context_mask[i] &= ~mask;
		    context_id = 4 * (32 * i + j);
		    return context_id;
		}
		mask >>= 1;
	    }
	}
    }
    /* return 0 if no context id found.  The calling routine should 
       check for this and generate the appropriate error code */
    return context_id;
}
void MPIR_Free_contextid( int context_id )
{
    int idx, bitpos;
    /* Convert the context id to the bit position */
    context_id <<= 2;       /* Remove the shift of a factor of four */
    idx = context_id % 32;
    bitpos = context_id / 32;

    context_mask[idx] |= (0x1 << bitpos);
}
#else
int MPIR_Get_contextid( MPI_Comm comm )
{
    /* FIXME. Not yet implemented.  See the MPICH 2 document for the 
       thread-safe algorithm. */
    return 0;
}
#endif

/*
 * Copy a communicator, including creating a new context and copying the
 * virtual connection tables and clearing the various fields.
 * Does *not* copy attributes.  If size is < the size of the input
 * communicator, copy only the first size elements.  If this process
 * is not a member, return a null pointer in outcomm_ptr.
 *
 * Used by comm_create, cart_create, graph_create, and dup_create 
 */
int MPIR_Comm_copy( MPID_Comm *comm_ptr, int size, MPID_Comm **outcomm_ptr )
{
    int mpi_errno = MPI_SUCCESS;
    int new_context_id;
    MPID_Comm *newcomm_ptr;

    /* Get a new context first.  We need this to be collective over the
       input communicator */
    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id */
    new_context_id = MPIR_Get_contextid( comm_ptr->handle );
    if (new_context_id == 0) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**toomanycomm", 0 );
	return mpi_errno;
    }

    if (comm_ptr->rank >= size) {
	*outcomm_ptr = 0;
	return MPI_SUCCESS;
    }

    /* We're left with the processes that will have a non-null communicator.
       Create the object, initialize the data, and return the result */

    newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!newcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    newcomm_ptr->ref_count = 1;
    newcomm_ptr->context_id = new_context_id;

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( comm_ptr->vcrt );
    newcomm_ptr->vcrt = comm_ptr->vcrt;
    newcomm_ptr->vcr  = comm_ptr->vcr;

    /* Set the sizes and ranks */
    newcomm_ptr->remote_size = comm_ptr->remote_size;
    newcomm_ptr->rank        = comm_ptr->rank;
    newcomm_ptr->local_size  = comm_ptr->local_size;
    
    /* More advanced version: if the group is available, dup it by 
       increasing the reference count */
    newcomm_ptr->local_group  = 0;
    newcomm_ptr->remote_group = 0;

    /* Inherit the error handler (if any) */
    newcomm_ptr->errhandler = comm_ptr->errhandler;
    if (comm_ptr->errhandler) {
	MPIU_Object_add_ref( comm_ptr->errhandler );
    }
    /* We could also inherit the communicator function pointer */
    newcomm_ptr->coll_fns = 0;

    /* We do *not* inherit any name */
    newcomm_ptr->name[0] = 0;

    *outcomm_ptr = newcomm_ptr;
    return MPI_SUCCESS;
}


