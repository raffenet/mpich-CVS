/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>

/*@
  MPID_Type_indexed - create an indexed datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes, otherwise
  they in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which vector is based

  Output Parameters:
. newtype - handle of new indexed datatype

  Return Value:
  0 on success, -1 on failure.

  This routine calls MPID_Dataloop_copy() to create the loops for this
  new datatype.  It calls MPIU_Handle_obj_alloc() to allocate space for the new
  datatype.
@*/

/* NOTE: This isn't a good example to copy, because the extent calculations
 * made me move code blocks into odd places. -- Rob Ross
 */
int MPID_Type_indexed(int count,
		      int *blocklength_array,
		      void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, new_loopsize, total_count, *iptr;
    char *curpos;
    MPI_Aint el_extent;

    MPID_Datatype *new_dtp, *old_dtp = NULL;
    struct MPID_Dataloop *dlp;
    MPI_Aint *aptr;

    MPI_Aint min_disp = 0, max_disp = 0;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }

    /* Note: handle and ref_count, the first two parameters in the datatype
     * structure, are filled in automatically by the handle allocation
     * function.
     */

    if (dispinbytes) new_dtp->combiner = MPI_COMBINER_HINDEXED;
    else new_dtp->combiner             = MPI_COMBINER_INDEXED;
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 0;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;

    /* get extent of old type; needed for new extent calculation */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN)
	el_extent = MPID_Datatype_get_basic_size(oldtype);
    else {
	/* get pointer to old datatype from handle */
	MPID_Datatype_get_ptr(oldtype, old_dtp); /* fills in old_dtp */
	el_extent = old_dtp->extent;
    }

    /* count # of elements and find min and max displacements */
    if (dispinbytes) {
	min_disp = ((MPI_Aint *) displacement_array)[0];
	max_disp = min_disp + (MPI_Aint) blocklength_array[0] * el_extent;
    }
    else {
	min_disp = (MPI_Aint) ((int *) displacement_array)[0] * el_extent;
	max_disp = (min_disp + (MPI_Aint) blocklength_array[0]) * el_extent;
    }
    total_count = blocklength_array[0];

    /* TODO: IS MY EXTENT CALCULATING RIGHT??? */
    /* TODO: ADD IN ALIGNMENT ISSUES IF ANY */

    for (i=1; i < count; i++) {
	total_count += blocklength_array[i];

	if (dispinbytes) /* hindexed */ {
	    if (((MPI_Aint *) displacement_array)[i] < min_disp)
		min_disp = ((MPI_Aint *) displacement_array)[i];
	    else if (((MPI_Aint *) displacement_array)[i] + blocklength_array[i] * el_extent > max_disp)
		max_disp = ((MPI_Aint *) displacement_array)[i] + blocklength_array[i] * el_extent;
	}
	else {
	    if ((MPI_Aint) ((int *) displacement_array)[i] * el_extent < min_disp)
		min_disp = ((int *) displacement_array)[i];
	    else if ((MPI_Aint) (((int *) displacement_array)[i] + blocklength_array[i]) * el_extent > max_disp)
		max_disp = (MPI_Aint) (((int *) displacement_array)[i] + blocklength_array[i]) * el_extent;
	}
    }

    /* builtins are handled differently than user-defined types because they
     * have no associated dataloop or datatype structure.
     */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN) {
	/* get old values directly from the handle using bit ops */
	int oldsize = MPID_Datatype_get_basic_size(oldtype);

	/* fill in remainder of new datatype */
	new_dtp->size           = total_count * oldsize;
	new_dtp->extent         = max_disp - min_disp; /* in bytes */
	new_dtp->has_sticky_ub  = 0;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->loopinfo_depth = 1;
	new_dtp->true_lb        = min_disp; /* ? */
	new_dtp->true_ub        = max_disp; /* ? */
	new_dtp->alignsize      = oldsize;
	new_dtp->n_elements     = -1; /* ??? */
	new_dtp->is_contig      = 0; /* ??? */

	/* allocate dataloop
	 * 
	 * Note: we allocate space for displacements as MPI_Aints, because
	 * that is how they are stored in the loop (despite being passed in
	 * as ints).
	 *
	 * We'll need to do a conversion in here too.
	 *
	 * TODO: PADDING???
	 */
	new_loopsize            = sizeof(struct MPID_Dataloop) + 
	    count * (sizeof(MPI_Aint) + sizeof(int));

	dlp                     = (struct MPID_Dataloop *)MPIU_Malloc(new_loopsize);
	if (dlp == NULL) assert(0);

	new_dtp->opt_loopinfo   = dlp;
	new_dtp->loopinfo       = dlp;
	new_dtp->loopsize       = new_loopsize;

	/* fill in dataloop, noting that this is a leaf.  no need to copy. */
	/* NOTE: element size in kind is off. */
	dlp->kind                       = DLOOP_KIND_INDEXED | DLOOP_FINAL_MASK | (oldsize << DLOOP_ELMSIZE_SHIFT);
	dlp->handle                     = new_dtp->handle;
	dlp->loop_params.i_t.count      = count;
	dlp->loop_params.i_t.u.handle   = oldtype;
	dlp->el_extent                  = oldsize; /* extent = size for basic types */
	dlp->el_size                    = oldsize;
    }
    else /* user-defined base type */ {
	new_dtp->size           = total_count * old_dtp->size; /* in bytes */
	new_dtp->extent         = max_disp - min_disp; /* in bytes */
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->loopinfo_depth = old_dtp->loopinfo_depth + 1;
	new_dtp->true_lb        = old_dtp->true_lb; /* WRONG */
	new_dtp->alignsize      = old_dtp->alignsize;
	new_dtp->n_elements     = -1; /* ??? */

	new_dtp->is_contig = 0; /* TODO: FIX THIS */

	/* allocate space for dataloop */
	new_loopsize = old_dtp->loopsize + sizeof(struct MPID_Dataloop) + 
	    count * (sizeof(MPI_Aint) + sizeof(int));

	dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loopsize);
	if (dlp == NULL) assert(0);

	new_dtp->loopinfo = dlp;
	new_dtp->opt_loopinfo = dlp;
	new_dtp->loopsize = new_loopsize;

	/* fill in top part of dataloop */
	dlp->kind                       = DLOOP_KIND_INDEXED | (old_dtp->size << DLOOP_ELMSIZE_SHIFT); /* WRONG I THINK */
	dlp->handle                     = new_dtp->handle; /* filled in by MPIU_Handle_obj_alloc */
	dlp->loop_params.i_t.count      = count;
	dlp->el_extent                  = el_extent;
	dlp->el_size                    = old_dtp->size;

	/* copy in old dataloop */
	curpos = (char *) dlp; /* NEED TO PAD? */
	curpos += new_loopsize - old_dtp->loopsize;

	MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	dlp->loop_params.i_t.u.dataloop = (struct MPID_Dataloop *) curpos;
    }

    /* copy in blocklength and displacement parameters (in that order) */
    curpos = (char *) dlp;
    curpos += sizeof(struct MPID_Dataloop);

    dlp->loop_params.i_t.blocksize_array = (int *) curpos;
    iptr = (int *) curpos;
    for (i=0; i < count; i++) {
	iptr[i] = blocklength_array[i];
    }
    curpos += count * sizeof(int);

    dlp->loop_params.i_t.offset_array = (MPI_Aint *) curpos;
    aptr = (MPI_Aint *) curpos;
    for (i=0; i < count; i++) {
	if (dispinbytes) /* hindexed */
	    aptr[i] = (MPI_Aint) ((MPI_Aint *) displacement_array)[i];
	else /* indexed */
	    aptr[i] = ((MPI_Aint) ((int *) displacement_array)[i]) * el_extent;
    }

    /* return handle to new datatype in last parameter */
    *newtype = new_dtp->handle;

    return MPI_SUCCESS;
}




