/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_indexed_block */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_indexed_block = PMPI_Type_create_indexed_block
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_indexed_block  MPI_Type_create_indexed_block
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_indexed_block as PMPI_Type_create_indexed_block
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_indexed_block PMPI_Type_create_indexed_block

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_indexed_block

/*@
   MPI_Type_create_indexed_block - create indexed block datatype

   Arguments:
+  int count - count
.  int blocklength - block length
.  int *array_of_displacements - displacements
.  MPI_Datatype oldtype - old datatype
-  MPI_Datatype *newtype - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_create_indexed_block(int count, int blocklength, int *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_indexed_block";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
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
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK);
    return MPI_SUCCESS;
}
