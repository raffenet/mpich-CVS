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

   FIXME: comm_create can't use this because the context id must be
   created separately from the communicator (creating the context
   is collective over oldcomm_ptr, but this routine may be called only
   by a subset of processes in the new communicator)
 */
int MPIR_Comm_create( MPID_Comm *oldcomm_ptr, MPID_Comm **newcomm_ptr )
{   
    int mpi_errno, new_context_id;
    MPID_Comm *newptr;

    newptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!newptr) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Comm_create", __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    *newcomm_ptr = newptr;
    MPIU_Object_set_ref( newptr, 1 );

    /* If there is a context id cache in oldcomm, use it here.  Otherwise,
       use the appropriate algorithm to get a new context id */
    newptr->context_id = new_context_id = 
	MPIR_Get_contextid( oldcomm_ptr->handle );
    newptr->attributes = 0;
    if (new_context_id == 0) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Comm_create", __LINE__, MPI_ERR_OTHER,
					  "**toomanycomm", 0 );
	return mpi_errno;
    }
    return 0;
}

/* Create a local intra communicator from the local group of the 
   specified intercomm.
   For the context id, use the intercomm's context id + 2.  <- FIXME?
 */
int MPIR_Setup_intercomm_localcomm( MPID_Comm *intercomm_ptr )
{
    MPID_Comm *localcomm_ptr;
    int mpi_errno;

    localcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!localcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Setup_intercomm_localcomm", __LINE__,
					  MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    MPIU_Object_set_ref( localcomm_ptr, 1 );
    localcomm_ptr->context_id = intercomm_ptr->context_id + 2;

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( intercomm_ptr->local_vcrt );
    localcomm_ptr->vcrt = intercomm_ptr->local_vcrt;
    localcomm_ptr->vcr  = intercomm_ptr->local_vcr;

    /* Save the kind of the communicator */
    localcomm_ptr->comm_kind   = MPID_INTRACOMM;
    
    /* Set the sizes and ranks */
    localcomm_ptr->remote_size = intercomm_ptr->local_size;
    localcomm_ptr->local_size  = intercomm_ptr->local_size;
    localcomm_ptr->rank        = intercomm_ptr->rank;

    /* More advanced version: if the group is available, dup it by 
       increasing the reference count */
    localcomm_ptr->local_group  = 0;
    localcomm_ptr->remote_group = 0;

    /* This is an internal communicator, so ignore */
    localcomm_ptr->errhandler = 0;
    
    /* No local functions for the collectives FIXME */
    localcomm_ptr->coll_fns = 0;

    /* We do *not* inherit any name */
    localcomm_ptr->name[0] = 0;

    localcomm_ptr->attributes = 0;

    intercomm_ptr->local_comm = localcomm_ptr;
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

#ifdef MPICH_DEBUG_INTERNAL
static void MPIR_PrintContextMask( FILE *fp )
{
    int i;
    int maxset=0;
    for (i=MAX_CONTEXT_MASK-1; i>=0; i--) {
	if (context_mask[i] != 0) break;
    }
    maxset = i;
    DBG_FPRINTF( fp, "Context mask: " );
    for (i=0; i<maxset; i++) {
	DBG_FPRINTF( fp, "%.8x ", context_mask[i] );
    }
    DBG_FPRINTF( fp, "\n" );
}
#endif
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
	context_mask[0] = 0xFFFFFFFC; 
	initialize_context_mask = 0;
    }
    memcpy( local_mask, context_mask, MAX_CONTEXT_MASK * sizeof(int) );
    MPIR_Nest_incr();
    NMPI_Allreduce( MPI_IN_PLACE, local_mask, MAX_CONTEXT_MASK, MPI_INT, 
		    MPI_BAND, comm );
    MPIR_Nest_decr();
    
    for (i=0; i<MAX_CONTEXT_MASK; i++) {
	if (local_mask[i]) {
	    /* There is a bit set in this word */
	    mask = 0x00000001;
	    /* This is a simple sequential search.  */
	    for (j=0; j<32; j++) {
		if (mask & local_mask[i]) {
		    /* Found the leading set bit */
		    context_mask[i] &= ~mask;
		    context_id = 4 * (32 * i + j);
#ifdef MPICH_DEBUG_INTERNAL
		    if (MPIR_IDebug("context")) {
			DBG_FPRINTF( stderr, "allocating contextid = %d\n", context_id ); 
			DBG_FPRINTF( stderr, "(mask[%d], bit %d\n", i, j );
		    }
#endif
		    return context_id;
		}
		mask <<= 1;
	    }
	}
    }
    /* return 0 if no context id found.  The calling routine should 
       check for this and generate the appropriate error code */
#ifdef MPICH_DEBUG_INTERNAL
    if (MPIR_IDebug("context"))
	MPIR_PrintContextMask( stderr );
#endif
    return context_id;
}

void MPIR_Free_contextid( int context_id )
{
    int idx, bitpos;
    /* Convert the context id to the bit position */
    /* printf( "Freeed id = %d\n", context_id ); */
    context_id >>= 2;       /* Remove the shift of a factor of four */
    idx    = context_id / 32;
    bitpos = context_id % 32;

    context_mask[idx] |= (0x1 << bitpos);

#ifdef MPICH_DEBUG_INTERNAL
    if (MPIR_IDebug("context")) {
	DBG_FPRINTF( stderr, "Freed context %d\n", context_id );
	DBG_FPRINTF( stderr, "mask[%d] bit %d\n", idx, bitpos );
    }
#endif
}

/* Get a context for a new intercomm.  There are two approaches 
   here (for MPI-1 codes only)
   (a) Each local group gets a context; the groups exchange, and
       the low value is accepted and the high one returned.  This
       works because the context ids are taken from the same pool.
   (b) Form a temporary intracomm over all processes and use that
       with the regular algorithm.
   
   In some ways, (a) is the better approach because it is the one that
   extends to MPI-2 (where the last step, returning the context, is 
   not used and instead separate send and receive context id value 
   are kept).  For this reason, we'll use (a).

   FIXME - This approach will not work for MPI-2
*/
int MPIR_Get_intercomm_contextid( MPID_Comm *comm_ptr )
{
    int context_id, remote_context_id, final_context_id;
    int tag = 31567; /* FIXME */

    if (!comm_ptr->local_comm) {
	/* Manufacture the local communicator */
	MPIR_Setup_intercomm_localcomm( comm_ptr );
    }

    /*printf( "local comm size is %d and intercomm local size is %d\n",
      comm_ptr->local_comm->local_size, comm_ptr->local_size );*/
    context_id = MPIR_Get_contextid( comm_ptr->local_comm->handle );
    if (context_id == 0) return 0;

    /* MPIC routine uses an internal context id.  The local leads (process 0)
       exchange data */
    remote_context_id = -1;
    if (comm_ptr->rank == 0) {
	MPIC_Sendrecv( &context_id, 1, MPI_INT, 0, tag,
		       &remote_context_id, 1, MPI_INT, 0, tag, 
		       comm_ptr->handle, MPI_STATUS_IGNORE );

	/* We need to do something with the context ids.  For 
	   MPI1, we can just take the min of the two context ids and
	   use that value.  For MPI2, we'll need to have separate
	   send and receive context ids - FIXME */
	if (remote_context_id < context_id)
	    final_context_id = remote_context_id;
	else 
	    final_context_id = context_id;
    }

    /* Make sure that all of the local processes now have this
       id */
    MPIR_Nest_incr();
    NMPI_Bcast( &final_context_id, 1, MPI_INT, 
		0, comm_ptr->local_comm->handle );
    MPIR_Nest_decr();
    /* If we did not choose this context, free it.  We won't do this
       once we have MPI2 intercomms (at least, not for intercomms that
       are not subsets of MPI_COMM_WORLD) - FIXME */
    if (final_context_id != context_id) {
	MPIR_Free_contextid( context_id );
    }
    /* printf( "intercomm context = %d\n", final_context_id ); */
    return final_context_id;
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
       use the appropriate algorithm to get a new context id.  Be careful
       of intercomms here */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	new_context_id = MPIR_Get_intercomm_contextid( comm_ptr );
    }
    else {
	new_context_id = MPIR_Get_contextid( comm_ptr->handle );
    }
    if (new_context_id == 0) {
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Comm_copy", __LINE__, MPI_ERR_OTHER, "**toomanycomm", 0 );
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
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Comm_copy", __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	return mpi_errno;
    }
    MPIU_Object_set_ref( newcomm_ptr, 1 );
    newcomm_ptr->context_id = new_context_id;

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( comm_ptr->vcrt );
    newcomm_ptr->vcrt = comm_ptr->vcrt;
    newcomm_ptr->vcr  = comm_ptr->vcr;

    /* Save the kind of the communicator */
    newcomm_ptr->comm_kind   = comm_ptr->comm_kind;
    
    /* If it is an intercomm, duplicate the local vcrt references */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	MPID_VCRT_Add_ref( comm_ptr->local_vcrt );
	newcomm_ptr->local_vcrt = comm_ptr->local_vcrt;
	newcomm_ptr->local_vcr  = comm_ptr->local_vcr;
	newcomm_ptr->local_comm = 0;
    }

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

    /* Start with no attributes on this communicator */
    newcomm_ptr->attributes = 0;
    *outcomm_ptr = newcomm_ptr;
    return MPI_SUCCESS;
}


int MPIR_Comm_release(MPID_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int inuse;
    
    MPIU_Object_release_ref( comm_ptr, &inuse);
    if (!inuse) {

        if (MPIR_Process.comm_parent == comm_ptr)
            MPIR_Process.comm_parent = NULL;

	/* Remove the attributes, executing the attribute delete routine.  Do this only if the attribute functions are defined. */
	if (MPIR_Process.attr_free && comm_ptr->attributes) {
	    mpi_errno = MPIR_Process.attr_free( comm_ptr->handle, 
						comm_ptr->attributes );
	}

	if (mpi_errno == MPI_SUCCESS) {
	    /* Free the VCRT */
	    MPID_VCRT_Release(comm_ptr->vcrt);
	
	    /* FIXME: For intercomms, free the local vcrt */

	    /* Free the context value */
	    MPIR_Free_contextid( comm_ptr->context_id );

	    /* Free the local and remote groups, if they exist */
            if (comm_ptr->local_group)
                MPIR_Group_release(comm_ptr->local_group);
            if (comm_ptr->remote_group)
                MPIR_Group_release(comm_ptr->remote_group);

	    /* FIXME - when we recover comm objects, many tests 
	       (such as c/grp_ctxt_comm/functional/MPI_Comm_create) fail */
  	    MPIU_Handle_obj_free( &MPID_Comm_mem, comm_ptr );  
	}
	else {
	    /* If the user attribute free function returns an error,
	       then do not free the communicator */
	    MPIU_Object_add_ref( comm_ptr );
	}
    }

    return mpi_errno;
}
