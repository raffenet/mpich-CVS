/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_free = PMPI_Type_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_free  MPI_Type_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_free as PMPI_Type_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_free PMPI_Type_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_free

/*@
   MPI_Type_free - free datatype

   Arguments:
.  MPI_Datatype *datatype - datatype

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Type_free(MPI_Datatype *datatype)
{
    static const char FCNAME[] = "MPI_Type_free";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    int inuse;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_FREE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_FREE);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( *datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    /* Check for built-in type */
	    if (HANDLE_GET_KIND(*datatype) == HANDLE_KIND_BUILTIN) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_TYPE,
						  "**dtypeperm", 0 );
	    }
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_FREE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPIU_Object_release_ref(datatype_ptr,&inuse);
    if (!inuse) {
	/* FIXME - We need to free the structure */
	if (datatype_ptr->free_fn) {
	    mpi_errno = (datatype_ptr->free_fn)( datatype_ptr );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_FREE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
    }
    *datatype = MPI_DATATYPE_NULL;

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_FREE);
    return MPI_SUCCESS;
}
