/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Type_create_struct */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_create_struct = PMPI_Type_create_struct
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_create_struct  MPI_Type_create_struct
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_create_struct as PMPI_Type_create_struct
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_create_struct PMPI_Type_create_struct

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_create_struct

/*@
   MPI_Type_create_struct - create struct datatype

   Input Parameters:
+ count - number of blocks (integer) --- also number of entries in arrays 
  array_of_types, array_of_displacements and array_of_blocklengths 
. array_of_blocklength - number of elements in each block (array of integer) 
. array_of_displacements - byte displacement of each block (array of integer) 
- array_of_types - type of elements in each block (array of handles to 
  datatype objects) 

   Output Parameter:
. newtype - new datatype (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_TYPE
@*/
int MPI_Type_create_struct(int count,
			   int array_of_blocklengths[],
			   MPI_Aint array_of_displacements[],
			   MPI_Datatype array_of_types[],
			   MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_create_struct";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_CREATE_STRUCT);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_CREATE_STRUCT);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int i;
	    MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_blocklengths, "blocklens", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_displacements, "indices", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(array_of_types, "types", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
		for (i=0; i < count; i++) {
		    MPIR_ERRTEST_ARGNEG(array_of_blocklengths[i], "blocklen", mpi_errno);
		    MPIR_ERRTEST_DATATYPE_NULL(array_of_types[i], "datatype", mpi_errno);
		    if (mpi_errno != MPI_SUCCESS) break; /* stop before we dereference the null type */
		    MPID_Datatype_get_ptr(array_of_types[i], datatype_ptr);
		    MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		}
	    }
            if (mpi_errno != MPI_SUCCESS) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_STRUCT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_struct(count,
				 array_of_blocklengths,
				 array_of_displacements,
				 array_of_types,
				 newtype);

    if (mpi_errno == MPI_SUCCESS) {
	int i, *ints;
	MPID_Datatype *new_dtp;

	ints = (int *) MPIU_Malloc((count + 1) * sizeof(int));
	assert(ints != NULL);
	ints[0] = count;
	for (i=0; i < count; i++) {
	    ints[i+1] = array_of_blocklengths[i];
	}

	MPID_Datatype_get_ptr(*newtype, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       MPI_COMBINER_STRUCT,
					       count+1, /* ints (count, blocklengths) */
					       count, /* aints (displacements) */
					       count, /* types */
					       ints,
					       array_of_displacements,
					       array_of_types);

	MPIU_Free(ints);
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_CREATE_STRUCT);
    if (mpi_errno == MPI_SUCCESS) return MPI_SUCCESS;
    else return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
}
