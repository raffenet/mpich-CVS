/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_compare */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_compare = PMPI_Comm_compare
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_compare  MPI_Comm_compare
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_compare as PMPI_Comm_compare
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_compare PMPI_Comm_compare

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_compare

/*@
   MPI_Comm_compare - compare two communicators

   Arguments:
+  MPI_Comm comm1 - communicator
.  MPI_Comm comm2 - communicator
-  int *result - result

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
EXPORT_MPI_API int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    static const char FCNAME[] = "MPI_Comm_compare";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr1 = NULL;
    MPID_Comm *comm_ptr2 = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_COMPARE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm1, comm_ptr1 );
    MPID_Comm_get_ptr( comm2, comm_ptr2 );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr1, mpi_errno );
            MPID_Comm_valid_ptr( comm_ptr2, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_COMPARE);
                return MPIR_Err_return_comm( comm_ptr1, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_COMPARE);
    return MPI_SUCCESS;
}

