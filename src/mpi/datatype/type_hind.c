/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_hindexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_hindexed = PMPI_Type_hindexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_hindexed  MPI_Type_hindexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_hindexed as PMPI_Type_hindexed
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_hindexed PMPI_Type_hindexed

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_hindexed

/*@
   MPI_Type_hindexed - short description

   Arguments:
+  int count - count
.  int blocklens[] - blocklens
.  MPI_Aint indices[] - indices
.  MPI_Datatype old_type - old datatype
-  MPI_Datatype *newtype - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
EXPORT_MPI_API int MPI_Type_hindexed(int count, int blocklens[], MPI_Aint indices[], MPI_Datatype old_type, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_hindexed";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_HINDEXED);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( old_type, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_HINDEXED);
    return MPI_SUCCESS;
}
