/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include <assert.h>

/* -- Begin Profiling Symbol Block for routine MPI_Type_indexed */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_indexed = PMPI_Type_indexed
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_indexed  MPI_Type_indexed
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_indexed as PMPI_Type_indexed
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Type_indexed PMPI_Type_indexed

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_indexed

/*@
    MPI_Type_indexed - Creates an indexed datatype

Input Parameters:
+ count - number of blocks -- also number of entries in indices and blocklens
. blocklens - number of elements in each block (array of nonnegative integers) 
. indices - displacement of each block in multiples of old_type (array of 
  integers)
- old_type - old datatype (handle) 

Output Parameter:
. newtype - new datatype (handle) 

.N Fortran

The indices are displacements, and are based on a zero origin.  A common error
is to do something like to following
.vb
    integer a(100)
    integer blens(10), indices(10)
    do i=1,10
         blens(i)   = 1
10       indices(i) = 1 + (i-1)*10
    call MPI_TYPE_INDEXED(10,blens,indices,MPI_INTEGER,newtype,ierr)
    call MPI_TYPE_COMMIT(newtype,ierr)
    call MPI_SEND(a,1,newtype,...)
.ve
expecting this to send 'a(1),a(11),...' because the indices have values 
'1,11,...'.   Because these are `displacements` from the beginning of 'a',
it actually sends 'a(1+1),a(1+11),...'.

If you wish to consider the displacements as indices into a Fortran array,
consider declaring the Fortran array with a zero origin
.vb
    integer a(0:99)
.ve

.N Errors
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
.N MPI_ERR_EXHAUSTED
@*/
int MPI_Type_indexed(int count,
		     int blocklens[],
		     int indices[],
		     MPI_Datatype old_type,
		     MPI_Datatype *newtype)
{
    static const char FCNAME[] = "MPI_Type_indexed";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_TYPE_INDEXED);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_TYPE_INDEXED);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int i;
	    MPID_Datatype *datatype_ptr = NULL;

            MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    MPIR_ERRTEST_DATATYPE_NULL(old_type, "datatype", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(blocklens, "blocklens", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(indices, "indices", mpi_errno);
	    if (mpi_errno == MPI_SUCCESS) {
 		if (HANDLE_GET_KIND(old_type) != HANDLE_KIND_BUILTIN) {
		    MPID_Datatype_get_ptr( old_type, datatype_ptr );
		    MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
		}
		/* verify that all blocklengths are > 0 (0 isn't ok is it?) */
		for (i=0; i < count; i++) MPIR_ERRTEST_ARGNEG(blocklens[i], "blocklen", mpi_errno);
	    }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_INDEXED);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    mpi_errno = MPID_Type_indexed(count,
				  blocklens,
				  indices,
				  0, /* displacements not in bytes */
				  old_type,
				  newtype);

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_Datatype *new_dtp;
	int i, *ints;

	ints = (int *) MPIU_Malloc((2*count + 1) * sizeof(int));
	if (ints == NULL) assert(0);

	ints[0] = count;

	for (i=0; i < count; i++) {
	    ints[i+1] = blocklens[i];
	}
	for (i=0; i < count; i++) {
	    ints[i + count + 1] = indices[i];
	}
	MPID_Datatype_get_ptr(*newtype, new_dtp);
	mpi_errno = MPID_Datatype_set_contents(new_dtp,
					       MPI_COMBINER_INDEXED,
					       2*count + 1, /* ints (count, blocklengths, displacements) */
					       0, /* aints (displacements) */
					       1, /* types */
					       ints,
					       NULL,
					       &old_type);
	MPIU_Free(ints);
    }

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_INDEXED);
	return MPI_SUCCESS;
    }

    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_type_indexed", "**mpi_type_indexed %d %p %p %D %p", count, blocklens, indices, old_type, newtype);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_INDEXED);
    return MPIR_Err_return_comm(0, FCNAME, mpi_errno);
}
