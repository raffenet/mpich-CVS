/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_group = PMPI_Comm_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_group  MPI_Comm_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_group as PMPI_Comm_group
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_group PMPI_Comm_group

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_group

/*@

MPI_Comm_group - Accesses the group associated with given communicator

Input Parameter:
. comm - Communicator

Output Parameter:
. group - Group in communicator

Using 'MPI_COMM_NULL' with 'MPI_Comm_group':

It is an error to use 'MPI_COMM_NULL' as one of the arguments to
'MPI_Comm_group'.  The relevant sections of the MPI standard are 

$(2.4.1 Opaque Objects)
A null handle argument is an erroneous 'IN' argument in MPI calls, unless an
exception is explicitly stated in the text that defines the function.

$(5.3.2. Group Constructors)
<no text in 'MPI_COMM_GROUP' allowing a null handle>

Previous versions of MPICH allow 'MPI_COMM_NULL' in this function.  In the
interests of promoting portability of applications, we have changed the
behavior of 'MPI_Comm_group' to detect this violation of the MPI standard.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
    static const char FCNAME[] = "MPI_Comm_group";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int i, lpid, n;
    MPID_Group *group_ptr;
    MPID_VCR   *local_vcr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_GROUP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_GROUP);
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
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GROUP);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Create a group if necessary and populate it with the 
       local process ids */
    if (!comm_ptr->local_group) {
	n = comm_ptr->local_size;
	mpi_errno = MPIR_Group_create( n, &group_ptr );
	if (mpi_errno)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
		"**mpi_comm_group", "**mpi_comm_group #comm %p", comm, group);
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GROUP );
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}

	/* Make sure that we get the correct group */
	if (comm_ptr->comm_kind == MPID_INTERCOMM) {
	    local_vcr = comm_ptr->local_vcr;
	}
	else
	    local_vcr = comm_ptr->vcr;
	
	for (i=0; i<n; i++) {
	    group_ptr->lrank_to_lpid[i].lrank = i;
	    (void) MPID_VCR_Get_lpid( local_vcr[i], &lpid );
	    group_ptr->lrank_to_lpid[i].lpid  = lpid;
	}
	group_ptr->size		 = n;
	group_ptr->rank		 = comm_ptr->rank;
	group_ptr->idx_of_first_lpid = -1;
	
	comm_ptr->local_group = group_ptr;
    }
    /* FIXME: Add a sanity check that the size of the group is the same as
       the size of the communicator.  This helps catch corrupted 
       communicators */

    *group = comm_ptr->local_group->handle;
    MPIU_Object_add_ref( comm_ptr->local_group );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_GROUP);
    return MPI_SUCCESS;
}

