/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_dup = PMPI_Type_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_dup  MPI_Type_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_dup as PMPI_Type_dup
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_dup PMPI_Type_dup

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_dup

/*@
   MPI_Type_dup - duplicate a datatype

   Arguments:
+  MPI_Datatype datatype - datatype
-  MPI_Datatype *newtype - new datatype

   Notes:
   This is an MPI-2 function.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_dup(MPI_Datatype datatype, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_dup";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_DUP);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_DUP);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DUP);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
	/* make a contig out of it so that we have a placeholder type, since
         * now we have to keep up with the contents and envelope data
	 */
	mpi_errno = MPID_Type_contiguous(1, datatype, newtype);
    }
    else {
	mpi_errno = MPID_Type_dup(datatype, newtype);
    }

    if (mpi_errno == MPI_SUCCESS) {
	MPID_Datatype *new_dtp;

	MPID_Datatype_get_ptr(*newtype, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       MPI_COMBINER_DUP,
					       0, /* ints */
					       0, /* aints */
					       1, /* types */
					       NULL,
					       NULL,
					       &datatype);
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_DUP);
    return MPI_SUCCESS;
}
