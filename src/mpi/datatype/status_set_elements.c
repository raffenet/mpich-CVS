/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Status_set_elements */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Status_set_elements = PMPI_Status_set_elements
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Status_set_elements  MPI_Status_set_elements
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Status_set_elements as PMPI_Status_set_elements
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Status_set_elements PMPI_Status_set_elements

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Status_set_elements

/*@
   MPI_Status_set_elements - status set elements

   Arguments:
+  MPI_Status *status - status
.  MPI_Datatype datatype - datatype
-  int count - count

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, 
			    int count)
{
    static const char FCNAME[] = "MPI_Status_set_elements";
    int mpi_errno = MPI_SUCCESS;
    int size;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_STATUS_SET_ELEMENTS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_STATUS_SET_ELEMENTS);
    /* Get handles to MPI objects. */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *datatype_ptr = NULL;
            MPIR_ERRTEST_INITIALIZED(mpi_errno);

	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    MPIR_ERRTEST_ARGNULL(status,"status",mpi_errno);
            /* Validate datatype_ptr */
	    MPID_Datatype_get_ptr( datatype, datatype_ptr );
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_STATUS_SET_ELEMENTS);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Datatype_get_size_macro(datatype, size);
    /* FIXME: The device may want something else here */
    status->count = count * size;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_STATUS_SET_ELEMENTS);
    return MPI_SUCCESS;
}
