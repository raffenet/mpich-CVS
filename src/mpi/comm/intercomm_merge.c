/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Intercomm_merge */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Intercomm_merge = PMPI_Intercomm_merge
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Intercomm_merge  MPI_Intercomm_merge
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Intercomm_merge as PMPI_Intercomm_merge
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Intercomm_merge PMPI_Intercomm_merge

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Intercomm_merge

/*@
MPI_Intercomm_merge - Creates an intracommuncator from an intercommunicator

Input Parameters:
+ comm - Intercommunicator
- high - Used to order the groups of the two intracommunicators within comm
  when creating the new communicator.  This is a boolean value; the group
  that sets high true has its processes ordered `after` the group that sets 
  this value to false.

Output Parameter:
. comm_out - Created intracommunicator

.N Fortran

Algorithm:
.vb
 1) Allocate contexts 
 2) Local and remote group leaders swap high values
 3) Determine the high value.
 4) Merge the two groups and make the intra-communicator
.ve

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_EXHAUSTED

.seealso: MPI_Intercomm_create, MPI_Comm_free
   MPI_Intercomm_merge - merge communicators
@*/
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    static const char FCNAME[] = "MPI_Intercomm_merge";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *newcomm_ptr;
    int  local_high, remote_high, i, j, new_size, new_context_id;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INTERCOMM_MERGE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INTERCOMM_MERGE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( intercomm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
	    if (comm_ptr && comm_ptr->comm_kind != MPID_INTERCOMM) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_COMM,
						  "**commnotinter", 0 );
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPIR_Nest_incr();

    /* Make sure that we have a local intercommunicator */
    if (!comm_ptr->local_comm) {
	/* Manufacture the local communicator */
	MPIR_Setup_intercomm_localcomm( comm_ptr );
    }

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int acthigh;
	    /* Check for consistent valus of high in each local group.
	     The Intel test suite checks for this; it is also an easy
	     error to make */
	    acthigh = high ? 1 : 0;   /* Clamp high into 1 or 0 */
	    NMPI_Allreduce( MPI_IN_PLACE, &acthigh, 1, MPI_INT, MPI_SUM,
			    comm_ptr->local_comm->handle );

	    /* acthigh must either == 0 or the size of the local comm */
	    if (acthigh != 0 && acthigh != comm_ptr->local_size) {
		MPIR_Nest_decr();
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, 
						  "**notsame",
						  "**notsame %s %s", "high", 
						  "MPI_Intercomm_merge" );
		
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* FIXME : We should not use the same context id here */
    /* Find the "high" value of the other group of processes.  This
       will be used to determine which group is ordered first in
       the generated communicator.  high is logical */
    local_high = high;
    if (comm_ptr->rank == 0) {
	NMPI_Sendrecv( &local_high, 1, MPI_INT, 0, 0, 
		       &remote_high, 1, MPI_INT, 0, 0, intercomm, 
		       MPI_STATUS_IGNORE );
    }

    /* All processes in the local group now need to get the 
       value of remote_high */
    NMPI_Bcast( &remote_high, 1, MPI_INT, 0, comm_ptr->local_comm->handle );
        
    /* If local_high and remote_high are the same, then order is arbitrary.
       we use the lpids of the rank 0 member of the local and remote
       groups to choose an order in this case. */
    if (local_high == remote_high) {
	int rpid, lpid;

	(void)MPID_VCR_Get_lpid( comm_ptr->vcr[0], &rpid );
	(void)MPID_VCR_Get_lpid( comm_ptr->local_vcr[0], &lpid );
	if (rpid < lpid) 
	    local_high = 1;
	else
	    local_high = 0;
    }

    newcomm_ptr = (MPID_Comm *)MPIU_Handle_obj_alloc( &MPID_Comm_mem );
    /* --BEGIN ERROR HANDLING-- */
    if (!newcomm_ptr) {
	MPIR_Nest_decr();
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_intercomm_merge", "**mpi_intercomm_merge %C %d %p", intercomm, high, newintracomm);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    /* --END ERROR HANDLING-- */

    new_size = comm_ptr->local_size + comm_ptr->remote_size;
    MPIU_Object_set_ref( newcomm_ptr, 1 );
    newcomm_ptr->context_id   = comm_ptr->context_id + 2; /* See below */
    newcomm_ptr->remote_size  = newcomm_ptr->local_size   = new_size;
    newcomm_ptr->rank         = -1;
    newcomm_ptr->local_group  = 0;
    newcomm_ptr->remote_group = 0;
    newcomm_ptr->comm_kind    = MPID_INTRACOMM;

    /* Now we know which group comes first.  Build the new vcr 
       from the existing vcrs */
    MPID_VCRT_Create( new_size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    if (local_high) {
	/* remote group first */
	j = 0;
	for (i=0; i<comm_ptr->remote_size; i++) {
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[j++] );
	}
	for (i=0; i<comm_ptr->local_size; i++) {
	    if (i == comm_ptr->rank) newcomm_ptr->rank = j;
	    MPID_VCR_Dup( comm_ptr->local_vcr[i], &newcomm_ptr->vcr[j++] );
	}
    }
    else {
	/* local group first */
	j = 0;
	for (i=0; i<comm_ptr->local_size; i++) {
	    if (i == comm_ptr->rank) newcomm_ptr->rank = j;
	    MPID_VCR_Dup( comm_ptr->local_vcr[i], &newcomm_ptr->vcr[j++] );
	}
	for (i=0; i<comm_ptr->remote_size; i++) {
	    MPID_VCR_Dup( comm_ptr->vcr[i], &newcomm_ptr->vcr[j++] );
	}
    }

    /* We've setup a temporary context id, based on the context id
       used by the intercomm.  This allows us to perform the allreduce
       operations within the context id algorithm, since we already
       have a valid (almost - see comm_create_hook) communicator.
    */
    /* printf( "About to get context id \n" ); fflush( stdout ); */
    new_context_id = MPIR_Get_contextid( newcomm_ptr->handle );
    /* --BEGIN ERROR HANDLING-- */
    if (new_context_id == 0) {
	MPIR_Nest_decr();
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**toomanycomm", 0 );
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_intercomm_merge", "**mpi_intercomm_merge %C %d %p", intercomm, high, newintracomm);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    /* --END ERROR HANDLING-- */
    /* printf( "Resetting contextid\n" ); fflush( stdout ); */
    newcomm_ptr->context_id = new_context_id;

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook( newcomm_ptr );

    *newintracomm = newcomm_ptr->handle;

    MPIR_Nest_decr();
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INTERCOMM_MERGE);
    return MPI_SUCCESS;
}
