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

/* NOTE: I think that this is in here so that we don't get two copies of it
 * in the case where we don't have weak symbols.
 */

/* MPIR_Type_get_elements
 *
 * This is called only when we encounter a type with a -1 as its element size,
 * indicating that there is more than one element type in the type.
 *
 * Returns the number of elements in bytes of one of these types.
 *
 * TODO: MOVE THIS INTO THE MPID TREE?
 */
static int MPIR_Type_get_elements(int *bytes_p, int count, MPI_Datatype datatype)
{
    int m_rem, m_count, type_size, type_elements, type_element_size;
    MPID_Datatype *datatype_ptr = NULL;
    
    /* figure out the type size, count of types, and remainder */

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
	type_size = MPID_Datatype_get_basic_size(datatype);
	type_elements = 1;
	type_element_size = type_size;
    }
    else {
	MPID_Datatype_get_ptr(datatype, datatype_ptr);

	type_size = datatype_ptr->size;
	type_elements = datatype_ptr->n_elements;
	type_element_size = datatype_ptr->element_size;
    }
	
    m_rem   = *bytes_p % type_size;
    m_count = *bytes_p / type_size;

    if (count != -1 && m_count > count) {
	/* no more than count of this type; add back to remainder */
	m_rem  += (m_count - count) * type_size;
	m_count = count;
    }

    /* -1 count is used on loop start to indicate that we want any number of types.
     * at this point we've stripped off all the "whole" types and should be down to
     * just a single partial of this type, so we reset the count so that we can't
     * end up with more than one type's worth of resulting elements.
     *
     * Q: IS THIS NECESSARY?  I THINK SO FOR THE STRUCT CASE...
     */
    if (count == -1) count = 1; /* reset count; we are on the last of this type now */

    /* based on calculated values either return or recurse */

    if (m_rem == 0 || type_element_size != -1) {
	/* if we have run out of bytes, or we know the element size, we can stop */
	*bytes_p = m_rem;
	return m_count * type_elements;
    }
    else {
	/* we have bytes left and still don't have a single element size; must recurse */
	int i, j, typecount = 0, *ints, nr_elements = 0, last_nr_elements;
	MPI_Aint *aints;
	MPI_Datatype *types;

	/* Establish locations of arrays; perhaps this should be a fn. call or this fn. should be an MPID one? */
	types = (MPI_Datatype *) (((char *) datatype_ptr->contents) + sizeof(MPID_Datatype_contents));
	ints  = (int *) (((char *) types) + datatype_ptr->contents->nr_types * sizeof(MPI_Datatype));
	aints = (MPI_Aint *) (((char *) ints) + datatype_ptr->contents->nr_ints * sizeof(int));

	switch (datatype_ptr->contents->combiner) {
	    case MPI_COMBINER_NAMED:
	    case MPI_COMBINER_DUP:
	    case MPI_COMBINER_RESIZED:
		return MPIR_Type_get_elements(bytes_p, count, *types);
		break;
	    case MPI_COMBINER_CONTIGUOUS:
	    case MPI_COMBINER_VECTOR:
	    case MPI_COMBINER_HVECTOR_INTEGER:
	    case MPI_COMBINER_HVECTOR:
		/* count is first in ints array */
		return MPIR_Type_get_elements(bytes_p, count * (*ints), *types);
		break;
	    case MPI_COMBINER_INDEXED_BLOCK:
		/* count is first in ints array, blocklength is second */
		return MPIR_Type_get_elements(bytes_p, count * ints[0] * ints[1], *types);
		break;
	    case MPI_COMBINER_INDEXED:
	    case MPI_COMBINER_HINDEXED_INTEGER:
	    case MPI_COMBINER_HINDEXED:
		for (i=0; i < (*ints); i++) {
		    typecount += ints[i+1]; /* add up all the blocklengths to get a max. # of the next type */
		}
		return MPIR_Type_get_elements(bytes_p, count * typecount, *types);
		break;
	    case MPI_COMBINER_STRUCT_INTEGER:
	    case MPI_COMBINER_STRUCT:
		/* In this case we can't simply multiply the count of the next type by the
		 * count of the current type, because we need to cycle through the types
		 * just as the struct would.  thus the nested loops.
		 *
		 * We need to keep going until we see a "0" elements returned or we run
		 * out of bytes.
		 */
		last_nr_elements = 1; /* dummy value */
		for (j=0; j < count && *bytes_p > 0 && last_nr_elements > 0; j++) {
		    /* recurse on each type */
		    for (i=0; i < (*ints); i++) {
			last_nr_elements = MPIR_Type_get_elements(bytes_p, ints[i+1], types[i]);
			nr_elements += last_nr_elements;
		    }
		}
		return nr_elements;
		break;
	    case MPI_COMBINER_SUBARRAY:
	    case MPI_COMBINER_DARRAY:
	    case MPI_COMBINER_F90_REAL:
	    case MPI_COMBINER_F90_COMPLEX:
	    case MPI_COMBINER_F90_INTEGER:
	    default:
		assert(0);
		return -1;
		break;
	}
    }
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
    int mpi_errno = MPI_SUCCESS, byte_count;
    MPI_Aint type_size, type_elements, type_element_size;
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
	byte_count = status->count;
	*elements = MPIR_Type_get_elements(&byte_count, -1, datatype);
    }

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
    return MPI_SUCCESS;
}








