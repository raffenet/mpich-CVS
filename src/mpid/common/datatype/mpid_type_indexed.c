/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

#undef MPID_TYPE_ALLOC_DEBUG

static int MPIDI_Type_indexed_count_contig(int count,
					   int *blocklength_array,
					   void *displacement_array,
					   int dispinbytes,
					   MPI_Aint old_extent);

static void MPIDI_Type_indexed_array_copy(int count,
					  int contig_count,
					  int *input_blocklength_array,
					  void *input_displacement_array,
					  int *output_blocklength_array,
					  MPI_Aint *output_displacement_array,
					  int dispinbytes,
					  MPI_Aint old_extent);
/*@
  MPID_Type_indexed - create an indexed datatype
 
  Input Parameters:
+ count - number of blocks in type
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes, otherwise
  they in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which new type is based

  Output Parameters:
. newtype - handle of new indexed datatype

  Return Value:
  0 on success, -1 on failure.
@*/

int MPID_Type_indexed(int count,
		      int *blocklength_array,
		      void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int is_builtin, old_is_contig;
    int i, contig_count;
    int el_sz, el_ct, old_ct, old_sz;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;
    MPI_Datatype el_type;

    MPID_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Type_indexed",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
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

    new_dtp->loopsize       = -1;
    new_dtp->loopinfo       = NULL;
    new_dtp->loopinfo_depth = -1;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (count == 0)
    {
	/* we are interpreting the standard here based on the fact that
	 * with a zero count there is nothing in the typemap.
	 *
	 * we handle this case explicitly to get it out of the way.
	 */
	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;

	new_dtp->alignsize    = 0;
	new_dtp->element_size = 0;
	new_dtp->eltype       = 0;

	new_dtp->size    = 0;
	new_dtp->lb      = 0;
	new_dtp->ub      = 0;
	new_dtp->true_lb = 0;
	new_dtp->true_ub = 0;
	new_dtp->extent  = 0;

	new_dtp->n_elements = 0;
	new_dtp->is_contig  = 1;

	mpi_errno = MPID_Dataloop_create_indexed(0,
						 NULL,
						 NULL,
						 0,
						 MPI_INT, /* dummy type */
						 &(new_dtp->loopinfo),
						 &(new_dtp->loopsize),
						 &(new_dtp->loopinfo_depth),
						 0);
	*newtype = new_dtp->handle;
	
	return mpi_errno;
    }
    else if (is_builtin)
    {
	/* builtins are handled differently than user-defined types because
	 * they have no associated dataloop or datatype structure.
	 */
	el_sz      = MPID_Datatype_get_basic_size(oldtype);
	old_sz     = el_sz;
	el_ct      = 1;
	el_type    = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;
    }
    else
    {
	/* user-defined base type (oldtype) */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	old_sz  = old_dtp->size;
	el_ct   = old_dtp->n_elements;
	el_type = old_dtp->eltype;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_extent    = old_dtp->extent;
	old_is_contig = old_dtp->is_contig;

	new_dtp->has_sticky_lb = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub = old_dtp->has_sticky_ub;
	new_dtp->element_size  = el_sz;
	new_dtp->eltype        = el_type;
    }

    /* priming for loop */
    old_ct = blocklength_array[0];
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[0] :
	(((MPI_Aint) ((int *) displacement_array)[0]) * old_extent);

    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[0],
			      eff_disp,
			      old_lb,
			      old_ub,
			      old_extent,
			      min_lb,
			      max_ub);

    /* determine min lb, max ub, and count of old types */
    for (i=1; i < count; i++)
    {
	MPI_Aint tmp_lb, tmp_ub;
	
	old_ct += blocklength_array[i]; /* add more oldtypes */
	
	eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
	    (((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
	
	/* calculate ub and lb for this block */
	MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength_array[i],
				  eff_disp,
				  old_lb,
				  old_ub,
				  old_extent,
				  tmp_lb,
				  tmp_ub);

	if (tmp_lb < min_lb) min_lb = tmp_lb;
	if (tmp_ub > max_ub) max_ub = tmp_ub;
    }

    new_dtp->size = old_ct * old_sz;

    new_dtp->lb      = min_lb;
    new_dtp->ub      = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent  = max_ub - min_lb;

    new_dtp->n_elements = old_ct * el_ct;

    /* new type is only contig for N types if it's all one big
     * block, its size and extent are the same, and the old type
     * was also contiguous.
     */
    contig_count = MPIDI_Type_indexed_count_contig(count,
						   blocklength_array,
						   displacement_array,
						   dispinbytes,
						   old_extent);

    if ((contig_count == 1) && (new_dtp->size == new_dtp->extent))
    {
	new_dtp->is_contig = old_is_contig;
    }
    else
    {
	new_dtp->is_contig = 0;
    }

    /* fill in dataloop */
    mpi_errno = MPID_Dataloop_create_indexed(count,
					     blocklength_array,
					     displacement_array,
					     dispinbytes,
					     oldtype,
					     &(new_dtp->loopinfo),
					     &(new_dtp->loopsize),
					     &(new_dtp->loopinfo_depth),
					     0);

    *newtype = new_dtp->handle;
    return mpi_errno;
}

/*@
   MPID_Dataloop_create_indexed

   Arguments:
+  int count
.  int *blocklength_array
.  void *displacement_array
.  int dispinbytes
.  MPI_Datatype oldtype
.  MPID_Dataloop **dlp_p
.  int *dlsz_p
.  int *dldepth_p
-  int flags

.N Errors
.N MPI_SUCCESS
@*/

int MPID_Dataloop_create_indexed(int count,
				 int *blocklength_array,
				 void *displacement_array,
				 int dispinbytes,
				 MPI_Datatype oldtype,
				 MPID_Dataloop **dlp_p,
				 int *dlsz_p,
				 int *dldepth_p,
				 int flags)
{
    int mpi_errno = MPI_SUCCESS, is_builtin;
    int i, old_loop_sz, new_loop_sz, old_loop_depth, blksz;
    int contig_count, old_type_count = 0;

    MPI_Aint old_extent;
    char *curpos;

    MPID_Datatype *old_dtp = NULL;
    struct MPID_Dataloop *new_dlp;

    /* if count is zero, handle with contig code, call it a int */
    if (count == 0)
    {
	mpi_errno = MPID_Dataloop_create_contiguous(0,
						    MPI_INT,
						    dlp_p,
						    dlsz_p,
						    dldepth_p,
						    flags);
	return mpi_errno;
    }

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (is_builtin)
    {
	old_extent     = MPID_Datatype_get_basic_size(oldtype);
	old_loop_sz    = 0;
	old_loop_depth = 0;
    }
    else
    {
	MPID_Datatype_get_ptr(oldtype, old_dtp);
	old_extent     = old_dtp->extent;
	old_loop_sz    = old_dtp->loopsize;
	old_loop_depth = old_dtp->loopinfo_depth;
    }

    for (i=0; i < count; i++)
    {
	old_type_count += blocklength_array[i];
    }

    contig_count = MPIDI_Type_indexed_count_contig(count,
						   blocklength_array,
						   displacement_array,
						   dispinbytes,
						   old_extent);
    assert(contig_count > 0);

    /* optimization:
     *
     * if contig_count == 1 and block starts at displacement 0,
     * store it as a contiguous rather than an indexed dataloop.
     */
    if ((contig_count == 1) &&
	((!dispinbytes && ((int *) displacement_array)[0] == 0) ||
	 (dispinbytes && ((MPI_Aint *) displacement_array)[0] == 0)))
    {
	mpi_errno = MPID_Dataloop_create_contiguous(old_type_count,
						    oldtype,
						    dlp_p,
						    dlsz_p,
						    dldepth_p,
						    flags);
	return mpi_errno;
    }

    /* optimization:
     *
     * if contig_count == 1 (and displacement != 0), store this as
     * a single element blockindexed rather than a lot of individual
     * blocks.
     */
    if (contig_count == 1)
    {
	mpi_errno = MPID_Dataloop_create_blockindexed(1,
						      old_type_count,
						      displacement_array,
						      dispinbytes,
						      oldtype,
						      dlp_p,
						      dlsz_p,
						      dldepth_p,
						      flags);
	return mpi_errno;
    }

    /* optimization:
     *
     * if block length is the same for all blocks, store it as a
     * blockindexed rather than an indexed dataloop.
     */
    blksz = blocklength_array[0];
    for (i=1; i < count; i++)
    {
	if (blocklength_array[i] != blksz)
	{
	    blksz--;
	    break;
	}
    }
    if (blksz == blocklength_array[0])
    {
	mpi_errno = MPID_Dataloop_create_blockindexed(count,
						      blocklength_array[0],
						      displacement_array,
						      dispinbytes,
						      oldtype,
						      dlp_p,
						      dlsz_p,
						      dldepth_p,
						      flags);
	return mpi_errno;
    }

    /* note: blockindexed looks for the vector optimization */

    /* TODO: optimization:
     *
     * if an indexed of a contig, absorb the contig into the blocklen array
     * and keep the same overall depth
     */

    /* otherwise storing as an indexed dataloop */

    new_loop_sz = sizeof(struct MPID_Dataloop) + 
	(contig_count * (sizeof(MPI_Aint) + sizeof(int))) +
	old_loop_sz;
    /* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */
    new_dlp = MPID_Dataloop_alloc(new_loop_sz);
    assert(new_dlp != NULL);

    if (is_builtin)
    {
	new_dlp->kind = DLOOP_KIND_INDEXED | DLOOP_FINAL_MASK;

	if (flags & MPID_DATALOOP_ALL_BYTES)
	{
	    /* blocklengths are modified below */
	    new_dlp->el_size   = 1;
	    new_dlp->el_extent = 1;
	    new_dlp->el_type   = MPI_BYTE;
	}
	else
	{
	    new_dlp->el_size   = old_extent;
	    new_dlp->el_extent = old_extent;
	    new_dlp->el_type   = oldtype;
	}

	new_dlp->loop_params.i_t.dataloop = NULL;
    }
    else
    {
	new_dlp->kind      = DLOOP_KIND_INDEXED;
	new_dlp->el_size   = old_dtp->size;
	new_dlp->el_extent = old_dtp->extent;
	new_dlp->el_type   = old_dtp->eltype;

	/* copy old dataloop and set pointer to it */
	curpos = (char *) new_dlp;
	curpos += (new_loop_sz - old_loop_sz);
	MPID_Dataloop_copy(curpos, old_dtp->loopinfo, old_dtp->loopsize);
	new_dlp->loop_params.i_t.dataloop = (struct MPID_Dataloop *) curpos;
    }

    new_dlp->loop_params.i_t.count        = contig_count;
    new_dlp->loop_params.i_t.total_blocks = old_type_count;

    /* copy in blocklength and displacement parameters (in that order)
     *
     * regardless of dispinbytes, we store displacements in bytes in loop.
     */
    curpos = (char *) new_dlp;
    curpos += sizeof(struct MPID_Dataloop);

    new_dlp->loop_params.i_t.blocksize_array = (int *) curpos;
    curpos += contig_count * sizeof(int);

    new_dlp->loop_params.i_t.offset_array = (MPI_Aint *) curpos;

    MPIDI_Type_indexed_array_copy(count,
				  contig_count,
				  blocklength_array,
				  displacement_array,
				  new_dlp->loop_params.i_t.blocksize_array,
				  new_dlp->loop_params.i_t.offset_array,
				  dispinbytes,
				  old_extent);

    if (is_builtin && (flags & MPID_DATALOOP_ALL_BYTES))
    {
	int *tmp_blklen_array = new_dlp->loop_params.i_t.blocksize_array;

	for (i=0; i < contig_count; i++)
	{
	    /* increase block lengths so they are in bytes */
	    tmp_blklen_array[i] *= old_extent;
	}
    }

    *dlp_p     = new_dlp;
    *dlsz_p    = new_loop_sz;
    *dldepth_p = old_loop_depth + 1;

    return MPI_SUCCESS;
}


/* MPIDI_Type_indexed_count_contig()
 *
 * Determines the actual number of contiguous blocks represented by the
 * blocklength/displacement arrays.  This might be less than count (as
 * few as 1).
 *
 * Extent passed in is for the original type.
 */
static int MPIDI_Type_indexed_count_contig(int count,
					   int *blocklength_array,
					   void *displacement_array,
					   int dispinbytes,
					   MPI_Aint old_extent)
{
    int i, contig_count = 1;
    int cur_blklen = blocklength_array[0];

    if (!dispinbytes)
    {
	int cur_tdisp = ((int *) displacement_array)[0];
	
	for (i = 1; i < count; i++)
	{
	    if (cur_tdisp + cur_blklen == ((int *) displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else
	    {
		cur_tdisp  = ((int *) displacement_array)[i];
		cur_blklen = blocklength_array[i];
		contig_count++;
	    }
	}
    }
    else
    {
	MPI_Aint cur_bdisp = ((MPI_Aint *) displacement_array)[0];
	
	for (i = 1; i < count; i++)
	{
	    if (cur_bdisp + cur_blklen * old_extent ==
		((MPI_Aint *) displacement_array)[i])
	    {
		/* adjacent to current block; add to block */
		cur_blklen += blocklength_array[i];
	    }
	    else
	    {
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
 *
 * Extent passed in is for the original type.
 */
static void MPIDI_Type_indexed_array_copy(int count,
					  int contig_count,
					  int *in_blklen_array,
					  void *in_disp_array,
					  int *out_blklen_array,
					  MPI_Aint *out_disp_array,
					  int dispinbytes,
					  MPI_Aint old_extent)
{
    int i, cur_idx = 0;

    out_blklen_array[0] = in_blklen_array[0];

    if (!dispinbytes)
    {
	 out_disp_array[0] = (MPI_Aint) ((int *) in_disp_array)[0];
	
	for (i = 1; i < count; i++)
	{
	    if (out_disp_array[cur_idx] + ((MPI_Aint) out_blklen_array[cur_idx]) * old_extent ==
		((MPI_Aint) ((int *) in_disp_array)[i]) * old_extent)
	    {
		/* adjacent to current block; add to block */
		out_blklen_array[cur_idx] += in_blklen_array[i];
	    }
	    else
	    {
		cur_idx++;
		assert(cur_idx < contig_count);
		out_disp_array[cur_idx] = ((MPI_Aint) ((int *) in_disp_array)[i]) * old_extent;
		out_blklen_array[cur_idx]  = in_blklen_array[i];
	    }
	}
    }
    else
    {
	out_disp_array[0] = ((MPI_Aint *) in_disp_array)[0];
	
	for (i = 1; i < count; i++)
	{
	    if (out_disp_array[cur_idx] + ((MPI_Aint) out_blklen_array[cur_idx]) * old_extent ==
		((MPI_Aint *) in_disp_array)[i])
	    {
		/* adjacent to current block; add to block */
		out_blklen_array[cur_idx] += in_blklen_array[i];
	    }
	    else
	    {
		cur_idx++;
		assert(cur_idx < contig_count);
		out_disp_array[cur_idx]   = ((MPI_Aint *) in_disp_array)[i];
		out_blklen_array[cur_idx] = in_blklen_array[i];
	    }
	}
    }
    return;
}
