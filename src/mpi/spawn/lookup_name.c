/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Lookup_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Lookup_name = PMPI_Lookup_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Lookup_name  MPI_Lookup_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Lookup_name as PMPI_Lookup_name
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Lookup_name PMPI_Lookup_name

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Lookup_name

/*@
   MPI_Lookup_name - lookup name

   Arguments:
+  char *service_name - service name
.  MPI_Info info - info
-  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
EXPORT_MPI_API int MPI_Lookup_name(char *service_name, MPI_Info info, char *port_name)
{
    static const char FCNAME[] = "MPI_Lookup_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_LOOKUP_NAME);
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
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_LOOKUP_NAME);
    return MPI_SUCCESS;
}
