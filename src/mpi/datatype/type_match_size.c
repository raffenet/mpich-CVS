/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_match_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_match_size = PMPI_Type_match_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_match_size  MPI_Type_match_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_match_size as PMPI_Type_match_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_match_size PMPI_Type_match_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_match_size

/*@
   MPI_Type_match_size - Find an MPI datatype matching a specified size

   Input Parameters:
+ typeclass - generic type specifier (integer) 
- size - size, in bytes, of representation (integer) 

   Output Parameter:
. type - datatype with correct type, size (handle) 

Notes:
'typeclass' is one of 'MPI_TYPECLASS_REAL', 'MPI_TYPECLASS_INTEGER' and 
'MPI_TYPECLASS_COMPLEX', corresponding to the desired typeclass. 
The function returns an MPI datatype matching a local variable of type 
'( typeclass, size )'. 


.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype)
{
    static const char FCNAME[] = "MPI_Type_match_size";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    static const char *tname = 0;
    static MPI_Datatype real_types[] = { MPI_FLOAT, MPI_DOUBLE
#ifdef HAVE_LONG_DOUBLE
					 ,MPI_LONG_DOUBLE
#endif
    };
    static MPI_Datatype int_types[] = { MPI_CHAR, MPI_SHORT, MPI_INT, 
					MPI_LONG
#ifdef HAVE_LONG_LONG
					, MPI_LONG_LONG
#endif
    };
    static MPI_Datatype complex_types[] = { MPI_COMPLEX, MPI_DOUBLE_COMPLEX };
    MPI_Datatype matched_datatype = MPI_DATATYPE_NULL;
    int i, tsize;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_MATCH_SIZE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_MATCH_SIZE);
    /* Get handles to MPI objects. */
    MPID_Datatype_get_ptr( *datatype, datatype_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    MPIR_ERRTEST_ARGNULL( datatype, "datatype", mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_MATCH_SIZE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* FIXME: Should make use of Fortran optional types (e.g., MPI_INTEGER2) */

    /* The following implementation follows the suggestion in the
       MPI-2 standard.  
       The version in the MPI-2 spec makes use of the Fortran optional types;
       currently, we don't support these from C (see mpi.h.in).  
       Thus, we look at the candidate types and make use of the first fit.
    */
    switch (typeclass) {
    case MPI_TYPECLASS_REAL:
	tname = "MPI_TYPECLASS_REAL";
	for (i=0; i<sizeof(real_types)/sizeof(MPI_Datatype); i++) {
	    if (real_types[i] == MPI_DATATYPE_NULL) { continue; }
	    PMPI_Type_size( real_types[i], &tsize );
	    if (tsize == size) {
		matched_datatype = real_types[i];
		break;
	    }
	}
	break;
    case MPI_TYPECLASS_INTEGER:
	tname = "MPI_TYPECLASS_INTEGER";
	for (i=0; i<sizeof(int_types)/sizeof(MPI_Datatype); i++) {
	    if (int_types[i] == MPI_DATATYPE_NULL) { continue; }
	    PMPI_Type_size( int_types[i], &tsize );
	    if (tsize == size) {
		matched_datatype = int_types[i];
		break;
	    }
	}
	break;
    case MPI_TYPECLASS_COMPLEX:
	tname = "MPI_TYPECLASS_COMPLEX";
	for (i=0; i<sizeof(complex_types)/sizeof(MPI_Datatype); i++) {
	    if (complex_types[i] == MPI_DATATYPE_NULL) { continue; }
	    PMPI_Type_size( complex_types[i], &tsize );
	    if (tsize == size) {
		matched_datatype = complex_types[i];
		break;
	    }
	}
	break;
    default:
	mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**typematchnoclass", 0);
	break;
    }

    if (mpi_errno == MPI_SUCCESS) {
	if (matched_datatype == MPI_DATATYPE_NULL) {
	    mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG, "**typematchsize", "**typematchsize %s %d", tname, size );
	}
	else {
	    *datatype = matched_datatype;
	}
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_MATCH_SIZE);
    if (mpi_errno)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_type_match_size", "**mpi_type_match_size %d %d %p", typeclass, size, datatype);
	return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    }
    return MPI_SUCCESS;
}
