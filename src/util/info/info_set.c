/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_set */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_set = PMPI_Info_set
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_set  MPI_Info_set
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_set as PMPI_Info_set
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_set PMPI_Info_set
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_set

/*@
    MPI_Info_set - Adds a (key,value) pair to info

Input Parameters:
+ info - info object (handle)
. key - key (string)
- value - value (string)

   Notes:

.N fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Info_set( MPI_Info info, char *key, char *value )
{
    static const char FCNAME[] = "MPI_Info_set";
    int mpi_errno = MPI_SUCCESS;
    MPID_Info *info_ptr=0, *curr_ptr, *prev_ptr;
    MPID_MPI_STATE_DECLS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_SET);
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
	    if (!value) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_INFO_VALUE, 
						  "**infovalnull", 0 );
	    }
	    else if (strlen(value) > MPI_MAX_INFO_VAL) {
		mpi_errno = MPIR_Err_create_code( MPI_ERR_INFO_VALUE, 
						  "**infovallong", 0 );
	    }
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_SET);
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
	    /* Key already present; replace value */
	    MPIU_Free(curr_ptr->value);  
	    curr_ptr->value = MPIU_Strdup(value);
	    break;
	}
	prev_ptr = curr_ptr;
	curr_ptr = curr_ptr->next;
    }

    if (!curr_ptr) {
	/* Key not present, insert value */
	curr_ptr         = (MPID_Info *)MPIU_Handle_obj_alloc( &MPID_Info_mem );
	if (!curr_ptr) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, 
					      "**nomem", 0 );
	    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	}
	/*printf( "Inserting new elm %x at %x\n", curr_ptr->id, prev_ptr->id );*/
	prev_ptr->next   = curr_ptr;
	curr_ptr->key    = MPIU_Strdup(key);
	curr_ptr->value  = MPIU_Strdup(value);
	curr_ptr->next   = 0;
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_SET);
    return MPI_SUCCESS;
}
