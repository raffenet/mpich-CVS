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

static int MPIDI_Type_indexed_count_contig(int count,
					   int *blocklength_array,
					   void *displacement_array,
					   int dispinbytes,
					   int el_sz,
					   int el_extent);

static void MPIDI_Type_indexed_array_copy(int count,
					  int contig_count,
					  int *input_blocklength_array,
					  void *input_displacement_array,
					  int *output_blocklength_array,
					  MPI_Aint *output_displacement_array,
					  int dispinbytes,
					  int el_sz,
					  int el_extent);
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

int MPID_Type_indexed(int count,
		      int *blocklength_array,
		      void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, new_loopsize, el_count, contig_count;
    char *curpos;

    MPID_Datatype *new_dtp;
    struct MPID_Dataloop *dlp;

    MPI_Aint min_lb = 0, max_ub = 0, eff_disp, el_extent = 0, el_size = 0;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPID_Type_indexed", __LINE__, MPI_ERR_OTHER, "**nomem", 0);
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

    /* builtins are handled differently than user-defined types because they
     * have no associated dataloop or datatype structure.
     */
    if (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN) {
	/* get old values directly from the handle using bit ops */
	el_size = MPID_Datatype_get_basic_size(oldtype);
	el_extent = el_size; /* needed at bottom of function at the very least */

	el_count = blocklength_array[0]; /* also going to count # of oldtypes */
	if (dispinbytes)
	    eff_disp = ((MPI_Aint *) displacement_array)[0];
	else
	    eff_disp = ((MPI_Aint) ((int *) displacement_array)[0]) * el_size;

	min_lb = eff_disp;
	max_ub = min_lb + (MPI_Aint) blocklength_array[0] * el_size;
	
	for (i=1; i < count; i++) {
	    MPI_Aint tmp_lb, tmp_ub; /* mostly used for clarity; could remove */

	    el_count += blocklength_array[i]; /* add more oldtypes */

	    if (dispinbytes)
		eff_disp = ((MPI_Aint *) displacement_array)[i];
	    else
		eff_disp = ((MPI_Aint) ((int *) displacement_array)[i]) * el_size;
	    
	    /* calculate ub and lb for this block */
	    tmp_lb = eff_disp;
	    tmp_ub = eff_disp + (MPI_Aint) blocklength_array[i] * el_size;
	    if (tmp_lb < min_lb) min_lb = tmp_lb;
	    if (tmp_ub > max_ub) max_ub = tmp_ub;
	}

	/* fill in remainder of new datatype */
	if (count == 0) new_dtp->size = 0;
	else            new_dtp->size = el_count * el_size;
	new_dtp->has_sticky_ub  = 0;
	new_dtp->has_sticky_lb  = 0;
	new_dtp->loopinfo_depth = 1;

	new_dtp->lb             = min_lb;
	new_dtp->ub             = max_ub;
	new_dtp->true_lb        = min_lb;
	new_dtp->true_ub        = max_ub;
	new_dtp->extent         = max_ub - min_lb;
	new_dtp->alignsize      = el_size;
	new_dtp->n_elements     = el_count;
	new_dtp->element_size   = el_size;
	new_dtp->is_contig      = 0; /* ??? */
        new_dtp->eltype         = oldtype;

	contig_count = MPIDI_Type_indexed_count_contig(count,
						       blocklength_array,
						       displacement_array,
						       dispinbytes,
						       el_size,
						       el_extent);

	if (contig_count == 1 &&
	    ((!dispinbytes && ((int *) displacement_array)[0] == 0) ||
	     (dispinbytes && ((MPI_Aint *) displacement_array)[0] == 0)))
	{
	    /* optimization: allocate a contig dataloop instead */
	    int tot_blks = 0;

	    new_loopsize = sizeof(struct MPID_Dataloop);
	    dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loopsize);
	    if (dlp == NULL) assert(0);

	    new_dtp->loopinfo = dlp;
	    new_dtp->loopsize = new_loopsize;

	    dlp->kind      = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
	    dlp->handle    = new_dtp->handle;
	    dlp->el_extent = el_extent;
	    dlp->el_size   = el_size;
	    dlp->el_type   = oldtype;

	    /* count up total size of data */
	    for (i=0; i < count; i++) tot_blks += blocklength_array[i];
	    dlp->loop_params.c_t.count = tot_blks;

	    /* return handle to new datatype in last parameter */
	    *newtype = new_dtp->handle;

	    return MPI_SUCCESS;
	}
	else {
	    /* allocate indexed dataloop
	     * 
	     * Note: we allocate space for displacements as MPI_Aints, because
	     * that is how they are stored in the loop (despite being passed in
	     * as ints).
	     *
	     * We'll need to do a conversion in here too.
	     *
	     */
	    new_loopsize = sizeof(struct MPID_Dataloop) + contig_count * (sizeof(MPI_Aint) + sizeof(int));
	    
	    dlp = (struct MPID_Dataloop *)MPIU_Malloc(new_loopsize);
	    if (dlp == NULL) assert(0);
	    
	    new_dtp->loopinfo       = dlp;
	    new_dtp->loopsize       = new_loopsize;
	    
	    /* fill in dataloop, noting that this is a leaf.  no need to copy. */
	    dlp->kind                       = DLOOP_KIND_INDEXED | DLOOP_FINAL_MASK;
	    dlp->handle                     = new_dtp->handle;
	    dlp->loop_params.i_t.count      = contig_count;
	    dlp->el_extent                  = el_size; /* extent = size for basic types */
	    dlp->el_size                    = el_size;
	    dlp->el_type                    = oldtype;
	}
    }
    else /* user-defined base type */ {
	MPI_Aint el_lb, el_ub;
	MPID_Datatype *old_dtp;
	    
	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_size   = old_dtp->size;
	el_extent = old_dtp->extent;
	el_lb     = old_dtp->lb;
	el_ub     = old_dtp->ub;

	/* get some starting values for lb, ub */
	el_count = blocklength_array[0]; /* also going to count # of oldtypes */
	if (dispinbytes)
	    eff_disp = ((MPI_Aint *) displacement_array)[0];
	else
	    eff_disp = ((MPI_Aint) ((int *) displacement_array)[0]) * el_extent;
	
	/* MPID_DATATYPE_BLOCK_LB_UB() is defined in mpid_datatype.h */
	MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[0],
				  eff_disp,
				  el_lb,
				  el_ub,
				  el_extent,
				  min_lb,
				  max_ub);
	
	/* find smallest lb, largest ub */
	for (i=1; i < count; i++) {
	    MPI_Aint tmp_lb, tmp_ub; /* mostly used for clarity; could remove */

	    el_count += blocklength_array[i]; /* add more oldtypes */

	    if (dispinbytes)
		eff_disp = ((MPI_Aint *) displacement_array)[i];
	    else
		eff_disp = ((MPI_Aint) ((int *) displacement_array)[i]) * el_extent;
	    
	    /* calculate ub and lb for this block */
	    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
				      eff_disp,
				      el_lb,
				      el_ub,
				      el_extent,
				      tmp_lb,
				      tmp_ub);

	    if (tmp_lb < min_lb) min_lb = tmp_lb;
	    if (tmp_ub > max_ub) max_ub = tmp_ub;
	}

	if (count == 0) new_dtp->size = 0;
	else            new_dtp->size = el_count * el_size; /* in bytes */
	new_dtp->lb             = min_lb;
	new_dtp->ub             = max_ub;
	new_dtp->true_lb        = min_lb + (old_dtp->true_lb - el_lb);
	new_dtp->true_ub        = max_ub + (old_dtp->true_ub - el_ub);
	new_dtp->extent         = max_ub - min_lb;

	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->loopinfo_depth = old_dtp->loopinfo_depth + 1;
	new_dtp->alignsize      = old_dtp->alignsize;
	new_dtp->n_elements     = el_count * old_dtp->n_elements;
	new_dtp->element_size   = old_dtp->element_size;
        new_dtp->eltype         = old_dtp->eltype;

	new_dtp->is_contig = 0; /* TODO: FIX THIS */

	contig_count = MPIDI_Type_indexed_count_contig(count,
						       blocklength_array,
						       displacement_array,
						       dispinbytes,
						       el_size,
						       el_extent);
	if (contig_count == 1 &&
	    ((!dispinbytes && ((int *) displacement_array)[0] == 0) ||
	     (dispinbytes && ((MPI_Aint *) displacement_array)[0] == 0)))
	{
	    /* optimization: allocate a contig loop instead */
	    int tot_blks = 0;

	    new_loopsize = old_dtp->loopsize + sizeof(struct MPID_Dataloop);
	    dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loopsize);
	    if (dlp == NULL) assert(0);

	    new_dtp->loopinfo = dlp;
	    new_dtp->loopsize = new_loopsize;
	    
	    /* fill in top part of dataloop */
	    dlp->kind      = DLOOP_KIND_CONTIG;
	    dlp->handle    = new_dtp->handle; /* filled in by MPIU_Handle_obj_alloc */
	    dlp->el_extent = el_extent;
	    dlp->el_size   = el_size;
	    dlp->el_type   = new_dtp->eltype;
	    
	    /* count up total size of data */
	    for (i=0; i < count; i++) tot_blks += blocklength_array[i];
	    dlp->loop_params.c_t.count = tot_blks;

	    /* copy in old dataloop */
	    curpos = (char *) dlp;
	    curpos += new_loopsize - old_dtp->loopsize;
	    
	    MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	    dlp->loop_params.c_t.dataloop = (struct MPID_Dataloop *) curpos;

	    /* return handle to new datatype in last parameter */
	    *newtype = new_dtp->handle;

	    return MPI_SUCCESS;
	}
	else {
	    /* allocate space for dataloop */
	    new_loopsize = old_dtp->loopsize + sizeof(struct MPID_Dataloop) + 
		contig_count * (sizeof(MPI_Aint) + sizeof(int));
	    
	    dlp = (struct MPID_Dataloop *) MPIU_Malloc(new_loopsize);
	    if (dlp == NULL) assert(0);
	    
	    new_dtp->loopinfo = dlp;
	    new_dtp->loopsize = new_loopsize;
	    
	    /* fill in top part of dataloop */
	    dlp->kind                  = DLOOP_KIND_INDEXED;
	    dlp->handle                = new_dtp->handle; /* filled in by MPIU_Handle_obj_alloc */
	    dlp->loop_params.i_t.count = contig_count;
	    dlp->el_extent             = el_extent;
	    dlp->el_size               = el_size;
	    dlp->el_type               = new_dtp->eltype;
	    
	    /* copy in old dataloop */
	    curpos = (char *) dlp; /* NEED TO PAD? */
	    curpos += new_loopsize - old_dtp->loopsize;
	    
	    MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	    dlp->loop_params.i_t.dataloop = (struct MPID_Dataloop *) curpos;
	}
    }

    /* copy in blocklength and displacement parameters (in that order) */
    curpos = (char *) dlp;
    curpos += sizeof(struct MPID_Dataloop);

    dlp->loop_params.i_t.blocksize_array = (int *) curpos;
    curpos += contig_count * sizeof(int);

    dlp->loop_params.i_t.offset_array = (MPI_Aint *) curpos;

    MPIDI_Type_indexed_array_copy(count,
				  contig_count,
				  blocklength_array,
				  displacement_array,
				  dlp->loop_params.i_t.blocksize_array,
				  dlp->loop_params.i_t.offset_array,
				  dispinbytes,
				  el_size,
				  el_extent);

    dlp->loop_params.i_t.total_blocks = 0;

    /* count up total number of blocks */
    for (i=0; i < contig_count; i++) {
	dlp->loop_params.i_t.total_blocks += dlp->loop_params.i_t.blocksize_array[i];
    }

    /* return handle to new datatype in last parameter */
    *newtype = new_dtp->handle;

#ifdef MPID_TYPE_ALLOC_DEBUG
    MPIU_dbg_printf("(h)indexed type %x created.\n", new_dtp->handle);
#endif
    return MPI_SUCCESS;
}



/* MPIDI_Type_indexed_count_contig()
 *
 * Determines the actual number of contiguous blocks represented by the
 * blocklength/displacement arrays.  This might be less than count (as
 * few as 1).
 *
 * TODO: cut out some of the unnecessary math.
 */
static int MPIDI_Type_indexed_count_contig(int count,
					   int *blocklength_array,
					   void *displacement_array,
					   int dispinbytes,
					   int el_sz,
					   int el_extent)
{
    int i, contig_count = 1;
    int cur_blklen = blocklength_array[0];

    if (!dispinbytes) {
	int cur_tdisp = ((int *) displacement_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (cur_tdisp + cur_blklen == ((int *) displacement_array)[i]) {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else {
		cur_tdisp  = ((int *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    else {
	MPI_Aint cur_bdisp = ((MPI_Aint *) displacement_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (cur_bdisp + cur_blklen * el_extent == ((MPI_Aint *) displacement_array)[i]) {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else {
		cur_bdisp  = ((MPI_Aint *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    return contig_count;
}


/* MPIDI_Type_indexed_array_copy()
 *
 * Copies arrays into place, combining adjacent contiguous regions.
 */
static void MPIDI_Type_indexed_array_copy(int count,
					  int contig_count,
					  int *input_blocklength_array,
					  void *input_displacement_array,
					  int *output_blocklength_array,
					  MPI_Aint *output_displacement_array,
					  int dispinbytes,
					  int el_sz,
					  int el_extent)
{
    int i, cur_idx = 0;

    output_blocklength_array[0] = input_blocklength_array[0];

    if (!dispinbytes) {
	 output_displacement_array[0] = (MPI_Aint) ((int *) input_displacement_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (output_displacement_array[cur_idx] + ((MPI_Aint) output_blocklength_array[cur_idx]) * el_extent ==
		((MPI_Aint) ((int *) input_displacement_array)[i]) * el_extent)
	    {
		/* adjacent to current block; add to block */
		output_blocklength_array[cur_idx] += input_blocklength_array[i];
	    }
	    else {
		cur_idx++;
		assert(cur_idx < contig_count);
		output_displacement_array[cur_idx] = ((MPI_Aint) ((int *) input_displacement_array)[i]) * el_extent;
		output_blocklength_array[cur_idx]  = input_blocklength_array[i];
	    }
	}
    }
    else {
	output_displacement_array[0] = ((MPI_Aint *) input_displacement_array)[0];
	
	for (i = 1; i < count; i++) {
	    if (output_displacement_array[cur_idx] + ((MPI_Aint) output_blocklength_array[cur_idx]) * el_extent ==
		((MPI_Aint *) input_displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		output_blocklength_array[cur_idx] += input_blocklength_array[i];
	    }
	    else {
		cur_idx++;
		assert(cur_idx < contig_count);
		output_displacement_array[cur_idx] = ((MPI_Aint *) input_displacement_array)[i];
		output_blocklength_array[cur_idx]  = input_blocklength_array[i];
	    }
	}
    }
    return;
}


