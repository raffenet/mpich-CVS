/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_delete */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_delete = PMPI_Info_delete
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_delete  MPI_Info_delete
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_delete as PMPI_Info_delete
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_delete PMPI_Info_delete
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_delete

/*@
  MPI_Info_delete - Deletes a (key,value) pair from info

  Input Parameters:
+ info - info object (handle)
- key - key (string)

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N 
@*/
int MPI_Info_delete( MPI_Info info, char *key )
{
    static const char FCNAME[] = "MPI_Info_delete";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr=0, *prev_ptr, *curr_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_DELETE);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_DELETE);
    /* Get handles to MPI objects. */
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
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_DELETE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    prev_ptr = info_ptr;
    curr_ptr = info_ptr->next;

    while (curr_ptr) {
	if (!strncmp(curr_ptr->key, key, MPI_MAX_INFO_KEY)) {
	    MPIU_Free(curr_ptr->key);   
	    MPIU_Free(curr_ptr->value);
	    prev_ptr->next = curr_ptr->next;
	    MPIU_Handle_obj_free( &MPID_Info_mem, curr_ptr );
	    break;
	}
	prev_ptr = curr_ptr;
	curr_ptr = curr_ptr->next;
    }

    if (!curr_ptr)
    {
	/* If curr_ptr is not defined, we never found the key */
	mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_INFO_NOKEY, "**infonokey",
					  "**infonokey %s", key );
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_info_delete", "**mpi_info_delete %I %p", info, key);
	
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_DELETE);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }

    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_DELETE);
    return MPI_SUCCESS;
}
