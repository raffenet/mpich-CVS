/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_get_contents */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_get_contents = PMPI_Type_get_contents
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_get_contents  MPI_Type_get_contents
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_get_contents as PMPI_Type_get_contents
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_get_contents PMPI_Type_get_contents

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_get_contents

/*@
   MPI_Type_get_contents - get type contents

   Arguments:
+  MPI_Datatype datatype - datatype
.  int max_integers - max integers
.  int max_addresses - max addresses
.  int max_datatypes - max datatypes
.  int array_of_integers[] - integers
.  MPI_Aint array_of_addresses[] - addresses
-  MPI_Datatype array_of_datatypes[] - datatypes

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_get_contents(MPI_Datatype datatype,
			  int max_integers,
			  int max_addresses,
			  int max_datatypes,
			  int array_of_integers[],
			  MPI_Aint array_of_addresses[],
			  MPI_Datatype array_of_datatypes[])
{
    static const char FCNAME[] = "MPI_Type_get_contents";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_GET_CONTENTS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_GET_CONTENTS);
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
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_CONTENTS);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_get_contents(datatype,
				       max_integers,
				       max_addresses,
				       max_datatypes,
				       array_of_integers,
				       array_of_addresses,
				       array_of_datatypes);
    if (mpi_errno != MPI_SUCCESS) {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_CONTENTS);
	return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_GET_CONTENTS);
    return MPI_SUCCESS;
}



