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
  MPID_Type_vector - create a vector datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength - number of elements in each block
. stride - distance from beginning of one block to the next (see next
  parameter for units)
. strideinbytes - if nonzero, then stride is in bytes, otherwise stride
  is in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new vector datatype

  Return Value:
  0 on success, -1 on failure.

  This routine calls MPID_Dataloop_copy() to create the loops for this
  new datatype.  It calls MPIU_Handle_obj_alloc() to allocate space for the new
  datatype.
@*/
int MPID_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_Datatype *new_dtp;
    struct MPID_Dataloop *dlp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPID_Type_vector", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->contents     = 0;

    /* The remaining parameters are filled in based on whether oldtype is 
     * a builtin or not: loopinfo, loopsize, size, extent, has_sticky_ub,
     * has_sticky_lb, loopinfo_depth, true_lb, alignsize, n_elements, 
     * element_size.
     *
     * Plus of course the loopinfo itself must still be filled in.
     */

    /* builtins are handled differently than user-defined types because they
     * have no associated dataloop or datatype structure.
     */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN) {
	/* get old values directly from the handle using bit ops */
	int oldsize = MPID_Datatype_get_basic_size(oldtype);

	/* fill in remainder of new datatype */
	new_dtp->size           = oldsize * count * blocklength;
	new_dtp->has_sticky_ub  = 0;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->loopinfo_depth = 1;

	/* calculate lb, ub, true_lb, and true_ub, which vary widely based on +/- stride */
	if (count == 0 || blocklength == 0) {
	    new_dtp->true_lb = 0;
	    new_dtp->true_ub = 0;
	    new_dtp->lb      = 0;
	    new_dtp->ub      = 0;
	}
	else if (stride >= 0) {
	    new_dtp->true_lb     = 0;
	    new_dtp->lb          = 0;
	    if (strideinbytes) 
		new_dtp->true_ub = (count-1) * stride + blocklength * oldsize;
	    else
		new_dtp->true_ub = ((count-1) * stride + blocklength) * oldsize;
	    new_dtp->ub          = new_dtp->true_ub;
	}
	else /* negative stride */ {
	    new_dtp->true_ub     = blocklength * oldsize;
	    new_dtp->ub          = new_dtp->true_ub;
	    if (strideinbytes)
		new_dtp->true_lb = (count-1) * stride;
	    else
		new_dtp->true_lb = (count-1) * stride * oldsize;
	    new_dtp->lb          = new_dtp->true_lb;
	}

	new_dtp->extent         = new_dtp->ub - new_dtp->lb;
	new_dtp->alignsize      = oldsize;
	new_dtp->n_elements     = count * blocklength;
	new_dtp->element_size   = oldsize;
	new_dtp->is_contig      = ((stride == blocklength || count == 1) ? 1 : 0);
        new_dtp->eltype         = oldtype;

	/* allocate dataloop */
	new_dtp->loopsize       = sizeof(struct MPID_Dataloop);

	/* TODO: maybe create a dataloop allocation function that understands 
	 * the types???
	 */
	dlp                     = (struct MPID_Dataloop *)MPIU_Malloc(sizeof(struct MPID_Dataloop));
	if (dlp == NULL) assert(0);
	new_dtp->loopinfo       = dlp;

	/* fill in dataloop, noting that this is a leaf.  no need to copy. */
	dlp->kind                       = DLOOP_KIND_VECTOR | DLOOP_FINAL_MASK;
	dlp->handle                     = new_dtp->handle;
	dlp->loop_params.v_t.count      = count;
	dlp->loop_params.v_t.blocksize  = blocklength;
	if (strideinbytes)
	    dlp->loop_params.v_t.stride = stride; /* already in bytes */
	else
	    dlp->loop_params.v_t.stride = stride * oldsize; /* convert to bytes */
	dlp->el_extent                  = oldsize;
	dlp->el_size                    = oldsize;
    }
    else /* user-defined base type */ {
	int new_loopsize;
	MPID_Datatype *old_dtp;
	char *curpos;
	MPI_Aint eff_stride;

	/* get pointer to old datatype from handle */
	MPID_Datatype_get_ptr(oldtype, old_dtp); /* fills in old_dtp */

	/* fill in datatype */
	new_dtp->size           = old_dtp->size * count * blocklength;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->loopinfo_depth = old_dtp->loopinfo_depth + 1;

	/* calculate the effective stride; in bytes */
	if (strideinbytes) eff_stride = stride;
	else eff_stride = stride * old_dtp->extent;

	/* calculate lb, ub, true_lb, and true_ub */
	/* MPID_DATATYPE_VECTOR_LB_UB() defined in mpid_datatype.h */
	MPID_DATATYPE_VECTOR_LB_UB(count,
				   eff_stride,
				   blocklength,
				   old_dtp->lb,
				   old_dtp->ub,
				   old_dtp->extent,
				   new_dtp->lb,
				   new_dtp->ub);

	new_dtp->true_lb      = new_dtp->lb + (old_dtp->true_lb - old_dtp->lb);
	new_dtp->true_ub      = new_dtp->ub + (old_dtp->true_ub - old_dtp->ub);
	new_dtp->extent       = new_dtp->ub - new_dtp->lb;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = old_dtp->n_elements * count * blocklength;
	new_dtp->element_size = old_dtp->element_size;
        new_dtp->eltype       = old_dtp->eltype;

	if (old_dtp->is_contig && (stride == blocklength || count == 1)) new_dtp->is_contig = 1; /* ??? */
	else new_dtp->is_contig = 0;

	/* allocate space for dataloop */
	new_loopsize = old_dtp->loopsize + sizeof(struct MPID_Dataloop);
	dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loopsize);
	if (dlp == NULL) assert(0);

	new_dtp->loopinfo = dlp;
	new_dtp->loopsize = new_loopsize;

	/* fill in top part of dataloop */
	dlp->kind                       = DLOOP_KIND_VECTOR;
	dlp->handle                     = new_dtp->handle; /* filled in by MPIU_Handle_obj_alloc */
	dlp->loop_params.v_t.count      = count;
	dlp->loop_params.v_t.blocksize  = blocklength;
	if (strideinbytes)
	    dlp->loop_params.v_t.stride = stride; /* already in bytes */
	else
	    dlp->loop_params.v_t.stride = stride * old_dtp->extent; /* convert to bytes */
	dlp->el_extent                  = old_dtp->extent;
	dlp->el_size                    = old_dtp->size;

	/* copy in old dataloop */
	curpos = (char *) dlp; /* NEED TO PAD? */
	curpos += sizeof(struct MPID_Dataloop);

	MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	dlp->loop_params.v_t.dataloop = (struct MPID_Dataloop *) curpos;
    }

    /* return handle to new datatype in last parameter */
    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("(h)vector type %x created.\n", new_dtp->handle);
#endif
    return MPI_SUCCESS;
}
