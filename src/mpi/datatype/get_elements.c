/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_elements */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_elements = PMPI_Get_elements
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_elements  MPI_Get_elements
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_elements as PMPI_Get_elements
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Get_elements PMPI_Get_elements

/* Return the number of elements */
static int MPIR_Type_get_elements( int m_rem, MPID_Datatype *datatype_ptr )
{
#if 0
    int mpi_errno;

	/* MPI calls shouldn't be looking at loops for now? */
    if ( (datatype_ptr->loopinfo->kind & DATALOOP_KIND_MASK) == MPID_DTYPE_STRUCT) {
	/* This is the hard case; we must loop through the components of the 
	   datatype */
	/* NOT DONE */
	mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, "**notimpl", 0 );
	MPIR_Err_return_comm( 0, "Get_elements", mpi_errno );
    }
    else {
	/* Recusively apply the algorithm */
	/* NOT DONE */
	mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, "**notimpl", 0 );
	MPIR_Err_return_comm( 0, "Get_elements", mpi_errno );
    }
#endif
    return 0;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_elements

/*@
   MPI_Get_elements - get_elements

   Arguments:
+  MPI_Status *status - status
.  MPI_Datatype datatype - datatype
-  int *elements - elements

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, int *elements)
{
    static const char FCNAME[] = "MPI_Get_elements";
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint m_count, m_rem, type_size, type_elements, type_element_size;
    MPID_Datatype *datatype_ptr = NULL;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_ELEMENTS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ELEMENTS);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(elements, "elements", mpi_errno);
            /* Validate datatype_ptr */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno == MPI_SUCCESS) MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
	    }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
                return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   else
    /* just grab the pointer */
    MPID_Datatype_get_ptr(datatype, datatype_ptr);
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
	type_size = MPID_Datatype_get_basic_size(datatype);
	type_elements = 1;
	type_element_size = type_size;
    }
    else /* derived type */ {
	type_size = datatype_ptr->size;
	type_elements = datatype_ptr->n_elements;
	type_element_size = datatype_ptr->element_size;
    }

    if (type_size == 0) {
	if (status->count > 0)
	    (*elements) = MPI_UNDEFINED;
	else {
	    /* This is ambiguous.  However, discussions on MPI Forum
	       reached a consensus that this is the correct return 
	       value
	    */
	    (*elements) = 0;
	}
    }
    else {
	m_count = status->count / type_size;
	m_rem   = status->count % type_size;
	if (m_rem == 0) {
	    (*elements) = m_count * type_elements;
	}
	else if (type_element_size > 0) {
	    (*elements) = status->count / type_element_size;
	}
	else {
	    /* element_size < 0 indicates that there was more than one basic type used */
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, "**notimpl", 0 );
	    MPIR_Err_return_comm( 0, "Get_elements", mpi_errno );
#if 0
	    /* This is the hard case */
	    (*elements) = m_count * datatype_ptr->n_elements;
	    /* Recusively handle the remaining (m_rem) bytes. */
	    *elements += MPIR_Type_get_elements( m_rem, datatype_ptr );
#endif
	}
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
    return MPI_SUCCESS;
}


