/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

#if !defined(MPID_DEV_TYPE_CONTIGUOUS_HOOK)
#define MPID_DEV_TYPE_CONTIGUOUS_HOOK(a, b, c, mpi_errno_p_)    \
{                                                               \
    *(mpi_errno_p_) = MPI_SUCCESS;                              \
}
#endif

/*@
  MPID_Type_zerolen - create an empty datatype
 
  Input Parameters:
. none

  Output Parameters:
. newtype - handle of new contiguous datatype

  Return Value:
  MPI_SUCCESS on success, MPI error code on failure.
@*/

int MPID_Type_zerolen(MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_zerolen",
					 __LINE__, MPI_ERR_OTHER,
					 "**nomem", 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = NULL;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = NULL;

    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_size  = -1;
    new_dtp->dataloop_depth = -1;
    new_dtp->hetero_dloop       = NULL;
    new_dtp->hetero_dloop_size  = -1;
    new_dtp->hetero_dloop_depth = -1;
    
    new_dtp->size          = 0;
    new_dtp->has_sticky_ub = 0;
    new_dtp->has_sticky_lb = 0;
    new_dtp->lb            = 0;
    new_dtp->ub            = 0;
    new_dtp->true_lb       = 0;
    new_dtp->true_ub       = 0;
    new_dtp->extent        = 0;
    
    new_dtp->alignsize     = 0;
    new_dtp->element_size  = 0;
    new_dtp->eltype        = 0;
    new_dtp->n_elements    = 0;
    new_dtp->is_contig     = 1;

    MPID_DEV_TYPE_CONTIGUOUS_HOOK(0, MPI_BYTE, new_dtp, &mpi_errno);
    if (mpi_errno != MPI_SUCCESS)
    {   /* --BEGIN ERROR HANDLING-- */
        MPID_Datatype_free(new_dtp);
        goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    *newtype = new_dtp->handle;

  fn_fail:
    return mpi_errno;
}
