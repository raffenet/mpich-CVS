/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_extent */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_extent = PMPI_Type_extent
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_extent  MPI_Type_extent
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_extent as PMPI_Type_extent
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_extent PMPI_Type_extent

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_extent

/*@
   MPI_Type_extent - datatype extent

   Arguments:
+  MPI_Datatype datatype - datatype
-  MPI_Aint *extent - extent

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_extent(MPI_Datatype datatype, MPI_Aint *extent)
{
    static const char FCNAME[] = "MPI_Type_extent";
    int mpi_errno = MPI_SUCCESS;
    int type_extent;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_EXTENT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_EXTENT);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_EXTENT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Datatype_get_extent_macro(datatype, type_extent);
    *extent = type_extent;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_EXTENT);
    return MPI_SUCCESS;
}
