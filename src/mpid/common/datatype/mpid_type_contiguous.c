/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@
  MPID_Type_contiguous - create a contiguous datatype
 
  Input Parameters:
+ count - number of elements in the contiguous block
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new vector datatype

  Return Value:
  MPI_SUCCESS on success, -1 on failure.

  This routine calls MPID_Dataloop_create_struct() to create the loops for this
  new datatype.  It calls MPID_Datatype_new() to allocate space for the new
  datatype.
@*/
int MPID_Type_contiguous(int count,
			 MPI_Datatype oldtype,
			 MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_Datatype *new_dtp;
    struct MPID_Dataloop *dlp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPID_Type_contiguous", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = 0;

    /* NOTE: see mpid_type_vector.c for a more thoroughly commented MPID type fn. */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN) {
	int oldsize = MPID_Datatype_get_basic_size(oldtype);

	new_dtp->size           = count * oldsize;
	new_dtp->has_sticky_ub  = 0;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->loopinfo_depth = 1;
	new_dtp->true_lb        = 0;
	new_dtp->lb             = 0;
	new_dtp->true_ub        = count * oldsize;
	new_dtp->ub             = new_dtp->true_ub;
	new_dtp->extent         = new_dtp->ub - new_dtp->lb;
	new_dtp->alignsize      = oldsize; /* ??? */
	new_dtp->n_elements     = count;
	new_dtp->element_size   = oldsize;
	new_dtp->is_contig      = 1;
        new_dtp->eltype         = oldtype;

	/* allocate dataloop */
	new_dtp->loopsize       = sizeof(struct MPID_Dataloop);
	dlp                     = (struct MPID_Dataloop *)MPIU_Malloc(sizeof(struct MPID_Dataloop));
	if (dlp == NULL) assert(0);
	new_dtp->loopinfo       = dlp;

	/* fill in dataloop */
	dlp->kind                       = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
	dlp->handle                     = new_dtp->handle;
	dlp->loop_params.c_t.count      = count;
	dlp->el_size                    = oldsize;
	dlp->el_extent                  = oldsize;
    }
    else /* user-defined base type */ {
	int new_loopsize;
	MPID_Datatype *old_dtp;
	char *curpos;

	/* get pointer to old datatype from handle */
	MPID_Datatype_get_ptr(oldtype, old_dtp); /* fills in old_dtp */

	/* fill in datatype */
	new_dtp->size           = count * old_dtp->size;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->loopinfo_depth = old_dtp->loopinfo_depth + 1;
        new_dtp->eltype         = old_dtp->eltype;

	/* calculate lb, ub, true_lb, true_ub */
	/* MPID_DATATYPE_CONTIG_LB_UB() defined in mpid_datatype.h */
	MPID_DATATYPE_CONTIG_LB_UB(count, old_dtp->lb, old_dtp->ub, old_dtp->extent, new_dtp->lb, new_dtp->ub);

	new_dtp->true_lb    = new_dtp->lb + (old_dtp->true_lb - old_dtp->lb);
	new_dtp->true_ub    = new_dtp->ub + (old_dtp->true_ub - old_dtp->ub);
	new_dtp->extent     = new_dtp->ub - new_dtp->lb;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = count * old_dtp->n_elements;
	new_dtp->element_size = old_dtp->element_size;
	new_dtp->is_contig    = old_dtp->is_contig; /* ??? */

	/* allocate space for dataloop */
	new_loopsize = old_dtp->loopsize + sizeof(struct MPID_Dataloop);
	dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loopsize);
	if (dlp == NULL) assert(0);

	new_dtp->loopinfo       = dlp;
	new_dtp->loopsize       = new_loopsize;

	/* fill in top part of dataloop */
	dlp->kind                  = DLOOP_KIND_CONTIG;
	/* NOTE: new_dtp->handle is filled in by MPIU_Handle_obj_alloc() */
	dlp->handle                = new_dtp->handle;
	dlp->loop_params.c_t.count = count;
	dlp->el_extent             = old_dtp->extent;
	dlp->el_size               = old_dtp->size;

	/* copy in old dataloop */
	curpos = (char *) dlp; /* NEED TO PAD? */
	curpos += sizeof(struct MPID_Dataloop);

	MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	dlp->loop_params.c_t.dataloop = (struct MPID_Dataloop *) curpos;
    }

    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("contig type %x created.\n", new_dtp->handle);
#endif

    return MPI_SUCCESS;
}








