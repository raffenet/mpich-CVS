/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

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
    int context_id;
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
	/* FIXME - error */
	;
    }
    
    if (comm_ptr->rank == local_leader) {
	int remote_info[2], local_info[2];
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
	
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_CREATE);
    return MPI_SUCCESS;
}

