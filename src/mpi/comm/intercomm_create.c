/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_create = PMPI_Intercomm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_create  MPI_Intercomm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_create as PMPI_Intercomm_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Intercomm_create PMPI_Intercomm_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_create

/*@
   MPI_Intercomm_create - create an intercommunicator

   Arguments:
+  MPI_Comm local_comm - local communicator
.  int local_leader - local leader
.  MPI_Comm peer_comm - peer communicator
.  int remote_leader - remote leader
.  int tag - tag
-  MPI_Comm *newintercomm - new intercommunicator

   Notes:

   'peer_comm' is significant only for the process designated the 
   'local_leader' in the 'local_comm'.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
			 MPI_Comm peer_comm, int remote_leader, int tag, 
			 MPI_Comm *newintercomm)
{
    static const char FCNAME[] = "MPI_Intercomm_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *peer_comm_ptr = NULL;
    int context_id, final_context_id;
    int remote_size, *remote_lpids;
    int i;
    MPID_Comm *newcomm_ptr, *commworld_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INTERCOMM_CREATE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INTERCOMM_CREATE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( local_comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    if (comm_ptr && 
		(local_leader < 0 || local_leader >= comm_ptr->local_size)) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_RANK, "**ranklocal", 
					  "**ranklocal %d %d", 
					  local_leader, comm_ptr->local_size );
	    }
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Create the contexts.  Each group will have a context for sending 
       to the other group */
    context_id = MPIR_Get_contextid( local_comm );
    if (context_id == 0) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**toomanycomm", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    
    if (comm_ptr->rank == local_leader) {
	int remote_info[2], local_info[2];
	int *local_lpids, i;

	MPID_Comm_get_ptr( peer_comm, peer_comm_ptr );
#       ifdef HAVE_ERROR_CHECKING
	{
	    MPID_BEGIN_ERROR_CHECKS;
	    {
		MPID_Comm_valid_ptr( peer_comm_ptr, mpi_errno );
		if (peer_comm_ptr && 
		    (remote_leader < 0 || 
		     remote_leader >= peer_comm_ptr->local_size)) {
		    mpi_errno = MPIR_Err_create_code( MPI_ERR_RANK, 
						      "**rankremote", 
					  "**rankremote %d %d", 
					  local_leader, comm_ptr->local_size );
		}
		if (mpi_errno) {
		    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
		    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
		}
	    }
	    MPID_END_ERROR_CHECKS;
	}
#       endif /* HAVE_ERROR_CHECKING */

	/* Exchange information with my peer.  Use sendrecv */
	local_info[0] = context_id;
	local_info[1] = comm_ptr->local_size;

	NMPI_Sendrecv( local_info, 2, MPI_INT, remote_leader, tag,
		       remote_info, 2, MPI_INT, remote_leader, tag, 
		       peer_comm, MPI_STATUS_IGNORE );
	
	/* With this information, we can now send and receive the 
	   local process ids from the peer.  This works only
	   for local process ids within MPI_COMM_WORLD, so this
	   will need to be fixed for the MPI2 version - FIXME */
	
	remote_size = remote_info[1];
	remote_lpids = (int *)MPIU_Malloc( remote_size * sizeof(int) );
	if (!remote_lpids) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	local_lpids =  (int *)MPIU_Malloc( comm_ptr->local_size * sizeof(int) );
	if (!local_lpids) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	for (i=0; i<comm_ptr->local_size; i++) {
	    (void)MPID_VCR_Get_lpid( comm_ptr->vcr[i], &local_lpids[i] );
	}
	
	/* Exchange the lpid arrays */
	NMPI_Sendrecv( local_lpids, comm_ptr->local_size, MPI_INT, 
		       remote_leader, tag,
		       remote_lpids, remote_size, MPI_INT, 
		       remote_leader, tag, peer_comm, MPI_STATUS_IGNORE );

	MPIU_Free( local_lpids );

	/* Now, send all of our local processes the remote_lpids */
	NMPI_Bcast( &remote_size, 1, MPI_INT, local_leader, local_comm );
	NMPI_Bcast( remote_lpids, remote_size, MPI_INT, local_leader, 
		    local_comm );

	/* 
	 * It would be good to detect invalid intercommunicators, such
	 * as ones with overlapping groups (the Fortran tests in the
	 * Intel test suite can erroneously attempt to create such
	 * intercommunicators if too few processes are used for the tests).
	 */
	/* We need to do something with the context ids.  For 
	   MPI1, we can just take the min of the two context ids and
	   use that value.  For MPI2, we'll need to have separate
	   send and receive context ids - FIXME */
	if (remote_info[0] < context_id) {
	    final_context_id = remote_info[0];
	}
	else 
	    final_context_id = context_id;
	NMPI_Bcast( &final_context_id, 1, MPI_INT, local_leader, local_comm );
    }
    else {
	/* We're *not* the leader, so we wait for the broadcast of the 
	   lpid array */
	NMPI_Bcast( &remote_size, 1, MPI_INT, local_leader, local_comm );
	remote_lpids = (int *)MPIU_Malloc( remote_size * sizeof(int) );
	if (!remote_lpids) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	NMPI_Bcast( remote_lpids, remote_size, MPI_INT, local_leader, 
		    local_comm );
	NMPI_Bcast( &final_context_id, 1, MPI_INT, local_leader, local_comm );
    }

    /* If we did not choose this context, free it.  We won't do this
       once we have MPI2 intercomms (at least, not for intercomms that
       are not subsets of MPI_COMM_WORLD) - FIXME */
    if (final_context_id != context_id) {
	MPIR_Free_contextid( context_id );
    }

    /* All processes in the local_comm now build the communicator */

    newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    if (!newcomm_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    MPIU_Object_set_ref( newcomm_ptr, 1 );
    newcomm_ptr->attributes   = 0;
    newcomm_ptr->context_id   = final_context_id;
    newcomm_ptr->remote_size  = remote_size;
    newcomm_ptr->local_size   = comm_ptr->local_size;
    newcomm_ptr->rank         = comm_ptr->rank;
    newcomm_ptr->local_group  = 0;
    newcomm_ptr->remote_group = 0;
    newcomm_ptr->comm_kind    = MPID_INTERCOMM;
    
    /* FIXME: for MPI1, all process ids are relative to MPI_COMM_WORLD.
       For MPI2, we'll need to do something more complex */
    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( remote_size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<remote_size; i++) {
	/* For rank i in the new communicator, find the corresponding
	   rank in the comm world (FIXME FOR MPI2) */
	/* printf( "[%d] Remote rank %d has lpid %d\n", 
	   MPIR_Process.comm_world->rank, i, remote_lpids[i] ); */
	MPID_VCR_Dup( commworld_ptr->vcr[remote_lpids[i]], 
		      &newcomm_ptr->vcr[i] );
	}

    /* Setup the communicator's vc table: local group.  This is
     just a duplicate of the local_comm's group */
    MPID_VCRT_Create( comm_ptr->local_size, &newcomm_ptr->local_vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->local_vcrt, &newcomm_ptr->local_vcr );
    for (i=0; i<comm_ptr->local_size; i++) {
	MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->local_vcr[i] );
    }
	
    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook( newcomm_ptr );
    
    *newintercomm = newcomm_ptr->handle;

    MPIU_Free( remote_lpids );

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
    return MPI_SUCCESS;
}

