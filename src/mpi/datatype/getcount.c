/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_count */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_count = PMPI_Get_count
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_count  MPI_Get_count
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_count as PMPI_Get_count
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Get_count PMPI_Get_count
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_count

/*@
  MPI_Get_count - Gets the number of "top level" elements

Input Parameters:
+ status - return status of receive operation (Status) 
- datatype - datatype of each receive buffer element (handle) 

Output Parameter:
. count - number of received elements (integer) 
Notes:
If the size of the datatype is zero, this routine will return a count of
zero.  If the amount of data in 'status' is not an exact multiple of the 
size of 'datatype' (so that 'count' would not be integral), a 'count' of
'MPI_UNDEFINED' is returned instead.

.N fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Get_count( MPI_Status *status, 	MPI_Datatype datatype, int *count )
{
    static const char FCNAME[] = "MPI_Get_count";
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *datatype_ptr = NULL;
    int size;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_COUNT);
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
            if (!status) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
                            "**nullptr", "**nullptr %s", "status" );
            } 
            /* Validate datatype_ptr */
            MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_COUNT);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Check for correct number of bytes */
    MPID_Datatype_get_size_macro(datatype, size);
    if (size != 0) {
	if ((status->count % size) != 0)
	    (*count) = MPI_UNDEFINED;
	else
	    (*count) = status->count / size;
    }
    else {
	if (status->count > 0)
	    (*count) = MPI_UNDEFINED;
	else {
	    /* This is ambiguous.  However, discussions on MPI Forum
	       reached a consensus that this is the correct return 
	       value
	    */
	    (*count) = 0;
	}
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_COUNT);
    return MPI_SUCCESS;
}
