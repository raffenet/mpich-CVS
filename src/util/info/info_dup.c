/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_dup = PMPI_Info_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_dup  MPI_Info_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_dup as PMPI_Info_dup
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_dup PMPI_Info_dup
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_dup

/*@
    MPI_Info_dup - Returns a duplicate of the info object

Input Arguments:
. info - info object (handle)

Output Arguments:
. newinfo - duplicate of info object (handle)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
EXPORT_MPI_API int MPI_Info_dup( MPI_Info info, MPI_Info *newinfo )
{
    MPID_Info *info_ptr=0, *curr_old, *curr_new;
    static const char FCNAME[] = "MPI_Info_dup";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_DUP);
    /* Get handles to MPI objects. */
    MPID_Info_get_ptr( info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_DUP);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Note that this routine allocates info elements one at a time.
       In the multithreaded case, each allocation may need to acquire
       and release the allocation lock.  If that is ever a problem, we
       may want to add an "allocate n elements" routine and execute this
       it two steps: count and then allocate */
    curr_new        = (MPID_Info *)MPIU_Handle_obj_new( &MPID_Info_mem );
    curr_new->key   = 0;
    curr_new->value = 0;
    curr_new->next  = 0;

    curr_old        = info_ptr->next;
    while (curr_old) {
	curr_new->next = (MPID_Info *)MPIU_Handle_obj_new( &MPID_Info_mem );
	if (!curr_new->next) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	}
	curr_new	 = curr_new->next;
	curr_new->key	 = MPIU_Strdup(curr_old->key);
	curr_new->value	 = MPIU_Strdup(curr_old->value);
	curr_new->next	 = 0;
	
	curr_old	 = curr_old->next;
    }
    *newinfo = curr_new->id;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_DUP);
    return MPI_SUCCESS;
}
