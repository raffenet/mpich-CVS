/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_valuelen */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_valuelen = PMPI_Info_get_valuelen
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_valuelen  MPI_Info_get_valuelen
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_valuelen as PMPI_Info_get_valuelen
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_get_valuelen PMPI_Info_get_valuelen
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get_valuelen

/*@
    MPI_Info_get_valuelen - Retrieves the length of the value associated with 
    a key

    Input Arguments:
+ info - info object (handle)
- key - key (string)

    Output Arguments:
+ valuelen - length of value argument (integer)
- flag - true if key defined, false if not (boolean)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO_KEY
.N MPI_ERR_OTHER
@*/
int MPI_Info_get_valuelen( MPI_Info info, char *key, int *valuelen, int *flag )
{
    MPID_Info *curr_ptr, *info_ptr=0;
    static const char FCNAME[] = "MPI_Info_get_valuelen";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET_VALUELEN);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET_VALUELEN);
    MPID_Info_get_ptr( info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int keylen;

	    MPIR_ERRTEST_INITIALIZED(mpi_errno);

	    /* Check input arguments */
	    if (!key) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_INFO_KEY,
						  "**infokeynull", 0 );
	    }
	    else if ((keylen = (int)strlen(key)) > MPI_MAX_INFO_KEY) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_INFO_KEY,
						  "**infokeylong", 0 );
	    } else if (keylen == 0) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_INFO_KEY,
						  "**infokeyempty", 0 );
	    }
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
	if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
	    *valuelen = (int)strlen(curr_ptr->value);
	    *flag = 1;
	    break;
	}
	curr_ptr = curr_ptr->next;
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_VALUELEN);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_info_get_valuelen", "**mpi_info_get_valuelen %I %s %p %p", info, key, valuelen, flag);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_VALUELEN);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
