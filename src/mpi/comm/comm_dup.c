/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_dup = PMPI_Comm_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_dup  MPI_Comm_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_dup as PMPI_Comm_dup
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_dup PMPI_Comm_dup

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_dup

/*@
   MPI_Comm_dup - duplicate a communicator

   Arguments:
+  MPI_Comm comm - communicator
-  MPI_Comm *newcomm - new communicator

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    static const char FCNAME[] = "MPI_Comm_dup";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_DUP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_DUP);
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
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DUP);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Generate a new context value and a new communicator structure */
    mpi_errno = MPIR_Comm_create( comm_ptr, &newcomm_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DUP );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* Duplicate the VCRT references */
    MPID_VCRT_Add_ref( comm_ptr->vcrt );
    newcomm_ptr->vcrt = comm_ptr->vcrt;
    newcomm_ptr->vcr  = comm_ptr->vcr;

    /* Copy attributes, executing the attribute copy functions */
    /* FIXME: this should access the function through the perprocess
       structure to prevent comm_dup from forcing the linking of the
       attribute functions */
    mpi_errno = MPIR_Comm_attr_dup( comm_ptr, &newcomm_ptr->attributes );

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DUP);
    return MPI_SUCCESS;
}

