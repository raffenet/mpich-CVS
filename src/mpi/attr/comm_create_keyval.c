#if 0
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create_keyval */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create_keyval = PMPI_Comm_create_keyval
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create_keyval  MPI_Comm_create_keyval
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create_keyval as PMPI_Comm_create_keyval
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_create_keyval PMPI_Comm_create_keyval

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create_keyval

/*@
   MPI_Comm_create_keyval - short description

   Arguments:
+  MPI_Comm_copy_attr_function *comm_copy_attr_fn - copy function
.  MPI_Comm_delete_attr_function *comm_delete_attr_fn - delete function
.  int *comm_keyval - keyval
-  void *extra_state - extra state

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state)
{
    static const char FCNAME[] = "MPI_Comm_create_keyval";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_KEYVAL);
    return MPI_SUCCESS;
}
#endif
