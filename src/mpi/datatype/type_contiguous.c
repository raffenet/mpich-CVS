/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_contigous */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_contiguous = PMPI_Type_contiguous
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_contiguous  MPI_Type_contiguous
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_contiguous as PMPI_Type_contiguous
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_contiguous PMPI_Type_contiguous
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_contiguous

/*@
   MPI_Type_contiguous - type_contiguous

   Arguments:
+  int count - count
.  MPI_Datatype old_type - old datatype
-  MPI_Datatype *new_type_p - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_contiguous(int count,
			MPI_Datatype old_type,
			MPI_Datatype *new_type_p)
{
    static const char FCNAME[] = "MPI_Type_contiguous";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CONTIGUOUS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CONTIGUOUS);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

	    /* MPIR_ERRTEST_XXX macros defined in mpiimpl.h */
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE_NULL(old_type, "datatype", mpi_errno);
            if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(old_type, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
	    }
	    if (mpi_errno != MPI_SUCCESS) {
		MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_TYPE_CONTIGUOUS);
		return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_contiguous(count,
				     old_type,
				     new_type_p);

    if (mpi_errno == MPI_SUCCESS) {
	MPID_Datatype *new_dtp;

	MPID_Datatype_get_ptr(*new_type_p, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       MPI_COMBINER_CONTIGUOUS,
					       1, /* ints (count) */
					       0,
					       1,
					       &count,
					       NULL,
					       &old_type);
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CONTIGUOUS);
    if (mpi_errno == MPI_SUCCESS) return MPI_SUCCESS;
    else return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
}








