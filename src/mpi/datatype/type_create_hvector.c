/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_hvector */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_hvector = PMPI_Type_create_hvector
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_hvector  MPI_Type_create_hvector
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_hvector as PMPI_Type_create_hvector
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_hvector PMPI_Type_create_hvector

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_hvector

/*@
   MPI_Type_create_hvector - create hvector datatype

   Arguments:
+  int count - count
.  int blocklength - block length
.  MPI_Aint stride - stride (in bytes)
.  MPI_Datatype oldtype - old datatype
-  MPI_Datatype *newtype - new datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_create_hvector(int count,
			    int blocklength,
			    MPI_Aint stride,
			    MPI_Datatype oldtype,
			    MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_hvector";
    int mpi_errno = MPI_SUCCESS, ret;
    MPID_Datatype *datatype_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_HVECTOR);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( oldtype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */

	    if (count < 0) 
		mpi_errno = MPIR_Err_create_code( MPI_ERR_COUNT, "**countneg",
						  "**countneg %d", count );
	    if (blocklength < 0) 
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, "**argneg",
						  "**argneg %s %d", 
						  "blocklength", blocklength );
	    /* MPICH 1 code also checked for old type equal to MPI_UB or LB.
	       We may want to check on length 0 datatypes */

            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_HVECTOR);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    ret = MPID_Type_vector(count,
			   blocklength,
			   stride,
			   1, /* stride in bytes */
			   old_type,
			   newtype_p);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_HVECTOR);
    if (ret == MPI_SUCCESS) return MPI_SUCCESS;
    else return MPIR_Err_return_comm(0, FCNAME, ret);
}



