/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_struct */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_struct = PMPI_Type_struct
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_struct  MPI_Type_struct
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_struct as PMPI_Type_struct
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_struct PMPI_Type_struct

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_struct

/*@
   MPI_Type_struct - type_struct

   Arguments:
+  int count - count
.  int blocklens[] - blocklens
.  MPI_Aint indices[] - indices
.  MPI_Datatype old_types[] - old datatypes
-  MPI_Datatype *newtype - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_struct(int count,
		    int blocklens[],
		    MPI_Aint indices[],
		    MPI_Datatype old_types[],
		    MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_struct";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_STRUCT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_STRUCT);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int i;
	    MPID_Datatype *datatype_ptr;

            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    MPIR_ERRTEST_ARGNULL(blocklens, "blocklens", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(indices, "indices", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(old_types, "types", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		/* verify that all blocklengths are > 0 (0 isn't ok is it?) */
		for (i=0; i < count; i++) {
		    MPIR_ERRTEST_ARGNONPOS(blocklens[i], "blocklen", mpi_errno);
		    MPIR_ERRTEST_DATATYPE_NULL(old_types[i], "datatype", mpi_errno);
		    if (mpi_errno != MPI_SUCCESS) break; /* stop before we dereference the null type */
		    MPID_Datatype_get_ptr(old_types[i], datatype_ptr);
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		}
	    }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_STRUCT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_struct(count,
				 blocklens,
				 indices,
				 old_types,
				 newtype);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_STRUCT);
    if (mpi_errno == MPI_SUCCESS) return MPI_SUCCESS;
    else return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
}






