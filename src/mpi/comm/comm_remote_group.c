/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_remote_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_remote_group = PMPI_Comm_remote_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_remote_group  MPI_Comm_remote_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_remote_group as PMPI_Comm_remote_group
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_remote_group PMPI_Comm_remote_group

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_remote_group

/*@
   MPI_Comm_remote_group - remote communicator group

   Arguments:
+  MPI_Comm comm - communicator
-  MPI_Group *group - group

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
    static const char FCNAME[] = "MPI_Comm_remote_group";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int i, lpid, n;
    MPID_Group *group_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_REMOTE_GROUP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_REMOTE_GROUP);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_REMOTE_GROUP);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Create a group and populate it with the local process ids */
    n = comm_ptr->remote_size;
    mpi_errno = MPIR_Group_create( n, &group_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_REMOTE_GROUP );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    
    for (i=0; i<n; i++) {
	group_ptr->lrank_to_lpid[i].lrank = i;
	(void) MPID_VCR_Get_lpid( comm_ptr->vcr[i], &lpid );
	group_ptr->lrank_to_lpid[i].lpid  = lpid;
    }
    group_ptr->size		 = n;
    group_ptr->rank		 = MPI_UNDEFINED;
    group_ptr->idx_of_first_lpid = -1;

    *group = group_ptr->handle;
    /* ... end of body of routine ... */


    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_REMOTE_GROUP);
    return MPI_SUCCESS;
}

