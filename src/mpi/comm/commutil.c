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

/* forward reference */
int MPIR_Get_contextid( MPI_Comm comm );

/* Preallocated comm objects */
MPID_Comm MPID_Comm_builtin[MPID_COMM_N_BUILTIN];
MPID_Comm MPID_Comm_direct[MPID_COMM_PREALLOC];
MPIU_Object_alloc_t MPID_Comm_mem = { 0, 0, 0, 0, MPID_COMM, 
				      sizeof(MPID_Comm), MPID_Comm_direct,
                                      MPID_COMM_PREALLOC};

/* Create a new communicator with a context */
int MPIR_Comm_create( MPID_Comm *oldcomm_ptr, MPID_Comm **newcomm_ptr )
{   
    int mpi_errno, new_context_id;

    *newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!*newcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
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
 * in detail in the mpich2 coding document.  
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

#else
int MPIR_Get_contextid( MPI_Comm comm )
{
    /* Not yet implemented.  See the MPICH 2 document for the 
       thread-safe algorithm. */
    return 0;
}
#endif
