/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>

#undef MPID_TYPE_ALLOC_DEBUG

static void MPIDI_Type_blockindexed_array_copy(int count,
					       void *disp_array,
					       MPI_Aint *out_disp_array,
					       int dispinbytes,
					       MPI_Aint old_extent);
static int MPIDI_Type_blockindexed_count_contig(int count,
						int blklen,
						void *disp_array,
						int dispinbytes,
						MPI_Aint old_extent);

/*@
  MPID_Type_blockindexed - create a block indexed datatype
 
  Input Parameters:
+ count - number of blocks in type
. blocklength - number of elements in each block
. displacement_array - offsets of blocks from start of type (see next
  parameter for units)
. dispinbytes - if nonzero, then displacements are in bytes, otherwise
  they in terms of extent of oldtype
- oldtype - type (using handle) of datatype on which new type is based

  Output Parameters:
. newtype - handle of new block indexed datatype

  Return Value:
  MPI_SUCCESS on success, MPI error on failure.
@*/

int MPID_Type_blockindexed(int count,
			   int blocklength,
			   void *displacement_array,
			   int dispinbytes,
			   MPI_Datatype oldtype,
			   MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS, i;
    int is_builtin, contig_count, old_is_contig;
    MPI_Aint el_sz;
    MPI_Datatype el_type;
    MPI_Aint old_lb, old_ub, old_extent, old_true_lb, old_true_ub;
    MPI_Aint min_lb = 0, max_ub = 0, eff_disp;

    MPID_Datatype *new_dtp;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dtp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_vector", __LINE__,
					 MPI_ERR_OTHER, "**nomem", 0);
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

    new_dtp->dataloop_size       = -1;
    new_dtp->dataloop       = NULL;
    new_dtp->dataloop_depth = -1;

    is_builtin = (HANDLE_GET_KIND(oldtype) == HANDLE_KIND_BUILTIN);

    if (count == 0)
    {
	/* we are interpreting the standard here based on the fact that
	 * with a zero count there is nothing in the typemap.
	 *
	 * we handle this case explicitly to get it out of the way.
	 */
	new_dtp->size          = 0;
	new_dtp->has_sticky_ub = 0;
	new_dtp->has_sticky_lb = 0;

	new_dtp->alignsize    = 0;
	new_dtp->n_elements   = 0;
	new_dtp->element_size = 0;
	new_dtp->eltype       = 0;

	new_dtp->lb      = 0;
	new_dtp->ub      = 0;
	new_dtp->true_lb = 0;
	new_dtp->true_ub = 0;
	new_dtp->extent  = 0;

	new_dtp->is_contig  = 1;

	mpi_errno = MPID_Dataloop_create_blockindexed(0,
						      0,
						      NULL,
						      0,
						      MPI_INT, /* dummy type */
						      &(new_dtp->dataloop),
						      &(new_dtp->dataloop_size),
						      &(new_dtp->dataloop_depth),
						      0);
	if (mpi_errno == MPI_SUCCESS) {
	    /* heterogeneous dataloop representation */
	    mpi_errno = MPID_Dataloop_create_blockindexed(0,
							  0,
							  NULL,
							  0,
							  MPI_INT,
							  &(new_dtp->hetero_dloop),
							  &(new_dtp->hetero_dloop_size),
							  &(new_dtp->hetero_dloop_depth),
							  0);
	}

	*newtype = new_dtp->handle;
	return mpi_errno;
    }
    else if (is_builtin)
    {
	el_sz   = MPID_Datatype_get_basic_size(oldtype);
	el_type = oldtype;

	old_lb        = 0;
	old_true_lb   = 0;
	old_ub        = el_sz;
	old_true_ub   = el_sz;
	old_extent    = el_sz;
	old_is_contig = 1;

	new_dtp->size          = count * blocklength * el_sz;
	new_dtp->has_sticky_lb = 0;
	new_dtp->has_sticky_ub = 0;

	new_dtp->alignsize    = el_sz; /* ??? */
	new_dtp->n_elements   = count * blocklength;
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;
    }
    else
    {
	/* user-defined base type (oldtype) */
	MPID_Datatype *old_dtp;

	MPID_Datatype_get_ptr(oldtype, old_dtp);
	el_sz   = old_dtp->element_size;
	el_type = old_dtp->eltype;

	old_lb        = old_dtp->lb;
	old_true_lb   = old_dtp->true_lb;
	old_ub        = old_dtp->ub;
	old_true_ub   = old_dtp->true_ub;
	old_extent    = old_dtp->extent;
	old_is_contig = old_dtp->is_contig;

	new_dtp->size           = count * blocklength * old_dtp->size;
	new_dtp->has_sticky_lb  = old_dtp->has_sticky_lb;
	new_dtp->has_sticky_ub  = old_dtp->has_sticky_ub;

	new_dtp->alignsize    = old_dtp->alignsize;
	new_dtp->n_elements   = count * blocklength * old_dtp->n_elements;
	new_dtp->element_size = el_sz;
	new_dtp->eltype       = el_type;
    }
    
    /* priming for loop */
    eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[0] :
	(((MPI_Aint) ((int *) displacement_array)[0]) * old_extent);
    MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength,
			      eff_disp,
			      old_lb,
			      old_ub,
			      old_extent,
			      min_lb,
			      max_ub);

    /* determine new min lb and max ub */
    for (i=1; i < count; i++)
    {
	MPI_Aint tmp_lb, tmp_ub;

	eff_disp = (dispinbytes) ? ((MPI_Aint *) displacement_array)[i] :
	    (((MPI_Aint) ((int *) displacement_array)[i]) * old_extent);
	MPID_DATATYPE_BLOCK_LB_UB((MPI_Aint) blocklength,
				  eff_disp,
				  old_lb,
				  old_ub,
				  old_extent,
				  tmp_lb,
				  tmp_ub);

	if (tmp_lb < min_lb) min_lb = tmp_lb;
	if (tmp_ub > max_ub) max_ub = tmp_ub;
    }

    new_dtp->lb      = min_lb;
    new_dtp->ub      = max_ub;
    new_dtp->true_lb = min_lb + (old_true_lb - old_lb);
    new_dtp->true_ub = max_ub + (old_true_ub - old_ub);
    new_dtp->extent  = max_ub - min_lb;

    /* new type is contig for N types if it is all one big block,
     * its size and extent are the same, and the old type was also
     * contiguous.
     */
    if (old_is_contig && (new_dtp->size == new_dtp->extent))
    {
	contig_count = MPIDI_Type_blockindexed_count_contig(count,
							    blocklength,
							    displacement_array,
							    dispinbytes,
							    old_extent);
	new_dtp->is_contig = (contig_count == 1) ? 1 : 0;
    }
    else
    {
	new_dtp->is_contig = 0;
    }

    mpi_errno = MPID_Dataloop_create_blockindexed(count,
						  blocklength,
						  displacement_array,
						  dispinbytes,
						  oldtype,
						  &(new_dtp->dataloop),
						  &(new_dtp->dataloop_size),
						  &(new_dtp->dataloop_depth),
						  0);
    if (mpi_errno == MPI_SUCCESS) {
	/* heterogeneous dataloop representation */
	mpi_errno = MPID_Dataloop_create_blockindexed(count,
						      blocklength,
						      displacement_array,
						      dispinbytes,
						      oldtype,
						      &(new_dtp->hetero_dloop),
						      &(new_dtp->hetero_dloop_size),
						      &(new_dtp->hetero_dloop_depth),
						      0);
    }

    *newtype = new_dtp->handle;
    return mpi_errno;
}

/*@
   MPID_Dataloop_create_blockindexed - create blockindexed dataloop

   Arguments:
+  int count
.  void *displacement_array
.  int displacement_in_bytes (boolean)
.  MPI_Datatype old_type
.  MPID_Dataloop **output_dataloop_ptr
.  int output_dataloop_size
.  int output_dataloop_depth
-  int flags

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Dataloop_create_blockindexed(int count,
				      int blklen,
				      void *disp_array,
				      int dispinbytes,
				      MPI_Datatype oldtype,
				      MPID_Dataloop **dlp_p,
				      int *dlsz_p,
				      int *dldepth_p,
				      int flags)
{
    int mpi_errno, is_builtin, is_vectorizable = 1;
    int i, old_loop_sz, new_loop_sz, old_loop_depth;
    int contig_count;

    MPI_Aint old_extent, eff_disp0, eff_disp1, last_stride;
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
	old_loop_sz    = old_dtp->dataloop_size;
	old_loop_depth = old_dtp->dataloop_depth;
    }

    contig_count = MPIDI_Type_blockindexed_count_contig(count,
							blklen,
							disp_array,
							dispinbytes,
							old_extent);

    /* optimization:
     *
     * if contig_count == 1 and block starts at displacement 0,
     * store it as a contiguous rather than a blockindexed dataloop.
     */
    if ((contig_count == 1) &&
	((!dispinbytes && ((int *) disp_array)[0] == 0) ||
	 (dispinbytes && ((MPI_Aint *) disp_array)[0] == 0)))
    {
	mpi_errno = MPID_Dataloop_create_contiguous(count * blklen,
						    oldtype,
						    dlp_p,
						    dlsz_p,
						    dldepth_p,
						    flags);
	return mpi_errno;
    }

    /* optimization:
     *
     * if contig_count == 1 store it as a blockindexed with one
     * element rather than as a lot of individual blocks.
     */
    if (contig_count == 1)
    {
	/* adjust count and blklen and drop through */
	blklen *= count;
	count = 1;
    }

    /* optimization:
     *
     * if displacements start at zero and result in a fixed stride,
     * store it as a vector rather than a blockindexed dataloop.
     */
    eff_disp0 = (dispinbytes) ? ((MPI_Aint *) disp_array)[0] :
	(((MPI_Aint) ((int *) disp_array)[0]) * old_extent);

    if (count > 1 && eff_disp0 == (MPI_Aint) 0)
    {
	eff_disp1 = (dispinbytes) ? ((MPI_Aint *) disp_array)[1] :
	    (((MPI_Aint) ((int *) disp_array)[1]) * old_extent);
	last_stride = eff_disp1 - eff_disp0;

	for (i=2; i < count; i++) {
	    eff_disp0 = eff_disp1;
	    eff_disp1 = (dispinbytes) ? ((MPI_Aint *) disp_array)[i] :
		(((MPI_Aint) ((int *) disp_array)[1]) * old_extent);
	    if (eff_disp1 - eff_disp0 != last_stride) {
		is_vectorizable = 0;
		break;
	    }
	}
	if (is_vectorizable)
	{
	    mpi_errno = MPID_Dataloop_create_vector(count,
						    blklen,
						    last_stride,
						    1, /* strideinbytes */
						    oldtype,
						    dlp_p,
						    dlsz_p,
						    dldepth_p,
						    flags);
	    return mpi_errno;
	}
    }

    /* TODO: optimization:
     *
     * if displacements result in a fixed stride, but first displacement
     * is not zero, store it as a blockindexed (blklen == 1) of a vector.
     */

    /* TODO: optimization:
     *
     * if a blockindexed of a contig, absorb the contig into the blocklen
     * parameter and keep the same overall depth
     */

    /* otherwise storing as a blockindexed dataloop */

    /* Q: HOW CAN WE TELL IF IT IS WORTH IT TO STORE AS AN
     * INDEXED WITH FEWER CONTIG BLOCKS (IF CONTIG_COUNT IS SMALL)?
     */

    new_loop_sz = sizeof(struct MPID_Dataloop) + 
	(count * sizeof(MPI_Aint)) + old_loop_sz;
    /* TODO: ACCOUNT FOR PADDING IN LOOP_SZ HERE */

    new_dlp = MPID_Dataloop_alloc(new_loop_sz);
    /* --BEGIN ERROR HANDLING-- */
    if (!new_dlp)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
					 MPIR_ERR_RECOVERABLE,
					 "MPID_Dataloop_create_blockindexed",
					 __LINE__,
					 MPI_ERR_OTHER,
					 "**nomem",
					 0);
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    if (is_builtin)
    {
	new_dlp->kind = DLOOP_KIND_BLOCKINDEXED | DLOOP_FINAL_MASK;

	if (flags & MPID_DATALOOP_ALL_BYTES)
	{
	    blklen            *= old_extent;
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

	new_dlp->loop_params.bi_t.count     = count;
	new_dlp->loop_params.bi_t.dataloop  = NULL;
	new_dlp->loop_params.bi_t.blocksize = blklen;
    }
    else
    {
	new_dlp->kind = DLOOP_KIND_BLOCKINDEXED;
	new_dlp->el_size   = old_dtp->size;
	new_dlp->el_extent = old_dtp->extent;
	new_dlp->el_type   = old_dtp->eltype;

	new_dlp->loop_params.bi_t.count     = count;
	new_dlp->loop_params.bi_t.dataloop  = NULL;
	new_dlp->loop_params.bi_t.blocksize = blklen;

	/* copy old dataloop and set pointer to it */
	curpos = (char *) new_dlp;
	curpos += (new_loop_sz - old_loop_sz);
	MPID_Dataloop_copy(curpos, old_dtp->dataloop, old_dtp->dataloop_size);
	new_dlp->loop_params.bi_t.dataloop = (struct MPID_Dataloop *) curpos;
    }

    /* copy in displacement parameters
     *
     * regardless of dispinbytes, we store displacements in bytes in loop.
     */
    curpos = (char *) new_dlp;
    curpos += sizeof(struct MPID_Dataloop);
    new_dlp->loop_params.bi_t.offset_array = (MPI_Aint *) curpos;

    MPIDI_Type_blockindexed_array_copy(count,
				       disp_array,
				       (MPI_Aint *) curpos,
				       dispinbytes,
				       old_extent);

    *dlp_p     = new_dlp;
    *dlsz_p    = new_loop_sz;
    *dldepth_p = old_loop_depth + 1;

    return MPI_SUCCESS;
}

/* MPIDI_Type_blockindexed_array_copy
 *
 * Unlike the indexed version, this one does not compact adjacent
 * blocks.
 */
static void MPIDI_Type_blockindexed_array_copy(int count,
					       void *in_disp_array,
					       MPI_Aint *out_disp_array,
					       int dispinbytes,
					       MPI_Aint old_extent)
{
    int i;
    if (!dispinbytes)
    {
	for (i=0; i < count; i++)
	{
	    out_disp_array[i] =
		((MPI_Aint) ((int *) in_disp_array)[i]) * old_extent;
	}
    }
    else
    {
	for (i=0; i < count; i++)
	{
	    out_disp_array[i] = ((MPI_Aint *) in_disp_array)[i];
	}
    }
    return;
}
				
static int MPIDI_Type_blockindexed_count_contig(int count,
						int blklen,
						void *disp_array,
						int dispinbytes,
						MPI_Aint old_extent)
{
    int i, contig_count = 1;

    if (!dispinbytes)
    {
	int cur_tdisp = ((int *) disp_array)[0];

	for (i=1; i < count; i++)
	{
	    if (cur_tdisp + blklen != ((int *) disp_array)[i])
	    {
		contig_count++;
	    }
	    cur_tdisp = ((int *) disp_array)[i];
	}
    }
    else
    {
	int cur_bdisp = ((MPI_Aint *) disp_array)[0];

	for (i=1; i < count; i++)
	{
	    if (cur_bdisp + blklen * old_extent !=
		((MPI_Aint *) disp_array)[i])
	    {
		contig_count++;
	    }
	    cur_bdisp = ((MPI_Aint *) disp_array)[i];
	}
    }
    return contig_count;
}
