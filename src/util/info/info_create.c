/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_create = PMPI_Info_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_create  MPI_Info_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_create as PMPI_Info_create
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Info_create PMPI_Info_create
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_create

/*@
    MPI_Info_create - Creates a new info object

   Output Arguments:
. info - info object (handle)

   Notes:

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Info_create( MPI_Info *info )
{
    MPID_Info *info_ptr;
    static const char FCNAME[] = "MPI_Info_create";
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_CREATE);
    /* Get handles to MPI objects. */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_CREATE);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    info_ptr = (MPID_Info *)MPIU_Handle_obj_alloc( &MPID_Info_mem );
    if (!info_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_CREATE);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    *info	    = info_ptr->id;
    /* (info_ptr)->cookie = MPIR_INFO_COOKIE; */
    info_ptr->key    = 0;
    info_ptr->value  = 0;
    info_ptr->next   = 0;
    /* this is the first structure in this linked list. it is 
       always kept empty. new (key,value) pairs are added after it. */
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_CREATE);
    return MPI_SUCCESS;
}
