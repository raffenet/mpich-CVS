/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_nthkey */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_nthkey = PMPI_Info_get_nthkey
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_nthkey  MPI_Info_get_nthkey
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_nthkey as PMPI_Info_get_nthkey
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_get_nthkey PMPI_Info_get_nthkey
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_get_nthkey

/*@
    MPI_Info_get_nthkey - Returns the nth defined key in info

Input Arguments:
+ info - info object (handle)
- n - key number (integer)

Output Argument:
. keys - key (string).  The maximum number of characters is 'MPI_MAX_INFO_KEY'.

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
.N MPI_ERR_ARG
@*/
int MPI_Info_get_nthkey( MPI_Info info, int n, char *key )
{
    MPID_Info *curr_ptr, *info_ptr=0;
    int       nkeys;
    static const char FCNAME[] = "MPI_Info_get_nthkey";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET_NTHKEY);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET_NTHKEY);
    /* Get handles to MPI objects. */
    MPID_Info_get_ptr( info, info_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NTHKEY);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    curr_ptr = info_ptr->next;
    nkeys = 0;
    while (curr_ptr && nkeys != n)
    {
	curr_ptr = curr_ptr->next;
	nkeys++;
    }

    if (curr_ptr)
    {
	/* Success */
	MPIU_Strncpy( key, curr_ptr->key, MPI_MAX_INFO_KEY+1 );
	/* Eventually, we could remember the location of this key in 
	   the head using the key/value locations (and a union datatype?) */
    }	
    else
    {
	/* n is invalid */
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_ARG,
					  "**infonkey", "**infonkey %d %d", 
					  n, nkeys );
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_info_get_nthkey", "**mpi_info_get_nthkey %I %d %p", info, n, key);
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NTHKEY);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NTHKEY);
    return MPI_SUCCESS;
}
