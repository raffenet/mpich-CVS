/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_nkeys */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_nkeys = PMPI_Info_get_nkeys
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_nkeys  MPI_Info_get_nkeys
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_nkeys as PMPI_Info_get_nkeys
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_get_nkeys PMPI_Info_get_nkeys
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get_nkeys

/*@
    MPI_Info_get_nkeys - Returns the number of currently defined keys in info

Input Arguments:
. info - info object (handle)

Output Arguments:
. nkeys - number of defined keys (integer)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Info_get_nkeys( MPI_Info info, int *nkeys )
{
    MPI_Info info_ptr;
    int      n;
    static const char FCNAME[] = "MPI_Info_get_nkeys";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET_NKEYS);
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
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NKEYS);
                return MPIR_Err_return_comm( 0, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    info_ptr = info_ptr->next;
    n = 0;

    while (info_ptr) {
	info_ptr = info_ptr->next;
	n ++;
    }
    *nkeys = n;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NKEYS);
    return MPI_SUCCESS;
}
