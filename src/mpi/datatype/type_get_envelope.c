/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_envelope */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_envelope = PMPI_Type_get_envelope
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_envelope  MPI_Type_get_envelope
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_envelope as PMPI_Type_get_envelope
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_get_envelope PMPI_Type_get_envelope

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_envelope

/*@
   MPI_Type_get_envelope - get type envelope

   Arguments:
+  MPI_Datatype datatype - datatype
.  int *num_integers - num integers
.  int *num_addresses - num addresses
.  int *num_datatypes - num datatypes
-  int *combiner - combiner

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_envelope(MPI_Datatype datatype,
			  int *num_integers,
			  int *num_addresses,
			  int *num_datatypes,
			  int *combiner)
{
    static const char FCNAME[] = "MPI_Type_get_envelope";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_GET_ENVELOPE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_ENVELOPE);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_get_envelope(datatype,
				       num_integers,
				       num_addresses,
				       num_datatypes,
				       combiner);
    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_ENVELOPE);
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno,
				     MPIR_ERR_RECOVERABLE,
				     FCNAME,
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**mpi_type_get_envelope",
				     "**mpi_type_get_envelope %D %p %p %p %p",
				     datatype,
				     num_integers,
				     num_addresses,
				     num_datatypes,
				     combiner);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_ENVELOPE);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    /* --END ERROR HANDLING-- */
}



