/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_delete_attr */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_delete_attr = PMPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_delete_attr  MPI_Type_delete_attr
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_delete_attr as PMPI_Type_delete_attr
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_delete_attr PMPI_Type_delete_attr

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_delete_attr

/*@
   MPI_Type_delete_attr - delete type attribute

   Arguments:
+  MPI_Datatype type - type
-  int type_keyval - value

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_delete_attr(MPI_Datatype type, int type_keyval)
{
    static const char FCNAME[] = "MPI_Type_delete_attr";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_DELETE_ATTR);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DELETE_ATTR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DELETE_ATTR);
    return MPI_SUCCESS;
}
