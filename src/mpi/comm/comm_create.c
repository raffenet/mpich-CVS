/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create = PMPI_Comm_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create  MPI_Comm_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create as PMPI_Comm_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_create PMPI_Comm_create

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create

/*@
   MPI_Comm_create - create a new communicator

   Arguments:
+  MPI_Comm comm - communicator
.  MPI_Group group - group
-  MPI_Comm *newcomm - new communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    static const char FCNAME[] = "MPI_Comm_create";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int i, n;
    MPID_Comm *newcomm_ptr;
    MPID_Group *group_ptr;
    int        *mapping;
    MPID_MPI_STATE_DECLS;

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Create a new communicator from the specified group members */
    
    /* Make sure that the processes for this group are contained within
       the input communicator.  Also identify the mapping from the ranks of 
       the old communicator to the new communicator */
    n = group_ptr->size;
    

    /* Get the new communicator structure and context id */
    mpi_errno = MPIR_Comm_create( comm_ptr, &newcomm_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* Since the group has been provided, let the new communicator know
       about the group */
    newcomm_ptr->local_group  = group_ptr;
    newcomm_ptr->remote_group = group_ptr;
    MPID_Obj_add_ref( group_ptr );
    MPID_Obj_add_ref( group_ptr );

    /* Setup the communicator's vc table */
    MPID_VCRT_Create( n, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    for (i=0; i<n; i++) {
	/* For rank i in the new communicator, find the corresponding
	   rank in the input communicator */
	MPID_VCR_Dup( comm_ptr->vcr[mapping[i]], &newcomm_ptr->vcr[i] );
    }

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook( newcomm_ptr );
    /* ... end of body of routine ... */

    /* mpi_errno = MPID_Comm_create(); */
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
	return MPI_SUCCESS;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}

