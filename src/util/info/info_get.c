/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get = PMPI_Info_get
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get  MPI_Info_get
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get as PMPI_Info_get
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_get PMPI_Info_get
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get

/*@
    MPI_Info_get - Retrieves the value associated with a key

Input Parameters:
+ info - info object (handle)
. key - key (string)
- valuelen - length of value argument (integer)

Output Parameters:
+ value - value (string)
- flag - true if key defined, false if not (boolean)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Info_get(MPI_Info info, char *key, int valuelen, char *value, 
		 int *flag)
{
    MPID_Info *curr_ptr, *info_ptr=0;
    static const char FCNAME[] = "MPI_Info_get";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET);
    /* Get handles to MPI objects. */
    MPID_Info_get_ptr( info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    int keylen;
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
	    /* Check input arguments */
	    if (!key) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_INFO_KEY,
						  "**infokeynull", 0 );
	    }
	    else if ((keylen = strlen(key)) > MPI_MAX_INFO_KEY) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_INFO_KEY,
						  "**infokeylong", 0 );
	    } else if (keylen == 0) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_INFO_KEY,
						  "**infokeyempty", 0 );
	    }
	    if (valuelen <= 0) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
					  "**argneg", 2, "value", valuelen );
	    }
	    if (!value) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_INFO_VALUE, 
						  "**infovalnull", 0 );
	    }
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    curr_ptr = info_ptr->next;
    *flag = 0;

    while (curr_ptr) {
	if (!strcmp(curr_ptr->key, key)) {
	    strncpy(value, curr_ptr->value, valuelen);
	    value[valuelen] = '\0';
	    *flag = 1;
	    break;
	}
	curr_ptr = curr_ptr->next;
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET);
    return MPI_SUCCESS;
}
