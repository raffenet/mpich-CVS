/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Errhandler_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Errhandler_get = PMPI_Errhandler_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Errhandler_get  MPI_Errhandler_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Errhandler_get as PMPI_Errhandler_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Errhandler_get PMPI_Errhandler_get

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Errhandler_get

/*@
   MPI_Errhandler_get - get the error handler

   Arguments:
+  MPI_Comm comm - communicator
-  MPI_Errhandler *errhandler - error handler

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Errhandler_get";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ERRHANDLER_GET);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ERRHANDLER_GET);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_GET);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    MPIR_Nest_incr();
    mpi_errno = PMPI_Comm_get_errhandler( comm, errhandler );
    MPIR_Nest_decr();
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_GET);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    /* ... end of body of routine ... */
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ERRHANDLER_GET);
    return MPI_SUCCESS;
}

