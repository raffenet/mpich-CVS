/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "dataloop.h"

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
    int mpi_errno;

    if ( (datatype_ptr->loopinfo->kind & DATALOOP_KIND_MASK) == MPID_STRUCT) {
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
    int m_count, m_rem;
    MPID_Datatype *datatype_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ELEMENTS);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( datatype, datatype_ptr );
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
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* INCOMPLETE */
    /* Check for correct number of bytes */
    if (datatype_ptr->size == 0) {
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
	m_count = status->count / datatype_ptr->size;
	m_rem   = status->count % datatype_ptr->size;
	if (m_rem == 0) {
	    (*elements) = m_count * datatype_ptr->n_elements;
	}
	else if (datatype_ptr->element_size > 0) {
	    (*elements) = status->count / datatype_ptr->element_size;
	}
	else {
	    /* This is the hard case */
	    (*elements) = m_count * datatype_ptr->n_elements;
	    /* Recusively handle the remaining (m_rem) bytes. */
	    *elements += MPIR_Type_get_elements( m_rem, datatype_ptr );
	}
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
    return MPI_SUCCESS;
}
