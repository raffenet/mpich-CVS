/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>

static int DLOOP_Dataloop_create_struct_memory_error(void);
static int DLOOP_Dataloop_create_unique_type_struct(int count,
						    int *blklen_array,
						    DLOOP_Offset *disp_array,
						    DLOOP_Type *oldtype_array,
						    int type_pos,
						    DLOOP_Dataloop **dlp_p,
						    int *dlsz_p,
						    int *dldepth_p,
						    int flags);
static int DLOOP_Dataloop_create_basic_all_bytes_struct(
	       int count,
	       int *blklen_array,
	       DLOOP_Offset *disp_array,
	       DLOOP_Type *oldtype_array,
	       DLOOP_Dataloop **dlp_p,
	       int *dlsz_p,
	       int *dldepth_p,
	       int flags);
static int DLOOP_Dataloop_create_flattened_struct(int count,
						  int *blklen_array,
						  DLOOP_Offset *disp_array,
						  DLOOP_Type *oldtype_array,
						  DLOOP_Dataloop **dlp_p,
						  int *dlsz_p,
						  int *dldepth_p,
						  int flags);

/*@
  DLOOP_Dataloop_create_struct - create the dataloop representation for a
  struct datatype

  Input Parameters:
+ count - number of blocks in vector
. blklen_array - number of elements in each block
. disp_array - offsets of blocks from start of type in bytes
- oldtype_array - types (using handle) of datatypes on which vector is based

  Output Parameters:
+ dlp_p - pointer to address in which to place pointer to new dataloop
- dlsz_p - pointer to address in which to place size of new dataloop

  Return Value:
  MPI_SUCCESS on success, MPI errno on failure.
@*/
int PREPEND_PREFIX(Dataloop_create_struct)(int count,
					   int *blklen_array,
					   DLOOP_Offset *disp_array,
					   DLOOP_Type *oldtype_array,
					   DLOOP_Dataloop **dlp_p,
					   int *dlsz_p,
					   int *dldepth_p,
					   int flags)
{
    int mpi_errno, i, nr_basics = 0, nr_derived = 0, type_pos = 0;

    DLOOP_Type first_basic = MPI_DATATYPE_NULL,
	first_derived = MPI_DATATYPE_NULL;

    /* variables used in general case only */
    int loop_idx = 0, new_loop_sz = 0, new_loop_depth, old_loop_depth = 0;
    char *curpos;
    DLOOP_Dataloop *new_dlp;

    /* if count is zero, handle with contig code, call it a int */
    if (count == 0)
    {
	mpi_errno = PREPEND_PREFIX(Dataloop_create_contiguous)(0,
							       MPI_INT,
							       dlp_p,
							       dlsz_p,
							       dldepth_p,
							       flags);
	return mpi_errno;
    }

    /* browse the old types and characterize */
    for (i=0; i < count; i++)
    {
	if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB)
	{
	    int is_builtin;

	    is_builtin =
		(DLOOP_Handle_hasloop_macro(oldtype_array[i])) ? 0 : 1;

	    if (is_builtin)
	    {
		if (nr_basics == 0)
		{
		    first_basic = oldtype_array[i];
		    type_pos = i;
		}
		else if (oldtype_array[i] != first_basic)
		{
		    first_basic = MPI_DATATYPE_NULL;
		}
		nr_basics++;
	    }
	    else /* derived type */
	    {
		if (nr_derived == 0)
		{
		    first_derived = oldtype_array[i];
		    type_pos = i;
		}
		else if (oldtype_array[i] != first_derived)
		{
		    first_derived = MPI_DATATYPE_NULL;
		}
		nr_derived++;
	    }
	}
    }

    /* note on optimizations:
     *
     * because we handle LB, UB, and extent calculations as part of
     * the MPID_Datatype, we can safely ignore them in all our
     * calculations here.
     */

    /* optimization:
     *
     * if there were only MPI_LBs and MPI_UBs in the struct type,
     * treat it as a zero-element contiguous (just as count == 0).
     */
    if (nr_basics == 0 && nr_derived == 0)
    {
	mpi_errno = PREPEND_PREFIX(Dataloop_create_contiguous)(0,
							       MPI_INT,
							       dlp_p,
							       dlsz_p,
							       dldepth_p,
							       flags);
	return mpi_errno;
    }

    /* optimization:
     *
     * if there is only one unique instance of a type in the struct, treat it
     * as a blockindexed type.
     *
     * notes:
     *
     * if the displacement happens to be zero, the blockindexed code will
     * optimize this into a contig.
     */
    if (nr_basics + nr_derived == 1)
    {
	/* type_pos is index to only real type in array */
	mpi_errno = PREPEND_PREFIX(Dataloop_create_blockindexed)
	    (1, /* count */
	     blklen_array[type_pos],
	     &disp_array[type_pos],
	     1, /* displacement in bytes */
	     oldtype_array[type_pos],
	     dlp_p,
	     dlsz_p,
	     dldepth_p,
	     flags);

	return mpi_errno;
    }

    /* optimization:
     *
     * if there only one unique type (more than one instance) in the
     * struct, treat it as an indexed type.
     *
     * notes:
     * 
     * this will apply to a single type with an LB/UB, as those
     * are handled elsewhere.
     *
     */
    if (((nr_derived == 0) && (first_basic != MPI_DATATYPE_NULL)) ||
	((nr_basics == 0) && (first_derived != MPI_DATATYPE_NULL)))
    {
	return DLOOP_Dataloop_create_unique_type_struct(count,
							blklen_array,
							disp_array,
							oldtype_array,
							type_pos,
							dlp_p,
							dlsz_p,
							dldepth_p,
							flags);
    }

    /* optimization:
     *
     * if there are no derived types and caller indicated either a
     * homogeneous system or the "all bytes" conversion, convert
     * everything to bytes and use an indexed type.
     */
    if (nr_derived == 0 && ((flags & MPID_DATALOOP_HOMOGENEOUS) ||
			    (flags & MPID_DATALOOP_ALL_BYTES)))
    {
	return DLOOP_Dataloop_create_basic_all_bytes_struct(count,
							    blklen_array,
							    disp_array,
							    oldtype_array,
							    dlp_p,
							    dlsz_p,
							    dldepth_p,
							    flags);
    }

    /* optimization:
     *
     * if caller asked for homogeneous or all bytes representation,
     * flatten the type and store it as an indexed type so that
     * there are no branches in the dataloop tree.
     */
    if ((flags & MPID_DATALOOP_HOMOGENEOUS) ||
	     (flags & MPID_DATALOOP_ALL_BYTES))
    {
	return DLOOP_Dataloop_create_flattened_struct(count,
						      blklen_array,
						      disp_array,
						      oldtype_array,
						      dlp_p,
						      dlsz_p,
						      dldepth_p,
						      flags);
    }

    /* scan through types and gather derived type info */
    for (i=0; i < count; i++)
    {
	if (HANDLE_GET_KIND(oldtype_array[i]) != HANDLE_KIND_BUILTIN)
	{
	    int tmp_loop_depth, tmp_loop_sz;

	    DLOOP_Handle_get_loopdepth_macro(oldtype_array[i], tmp_loop_depth,
					     0);
	    DLOOP_Handle_get_loopsize_macro(oldtype_array[i], tmp_loop_sz,
					    0);

	    if (tmp_loop_depth > old_loop_depth)
	    {
		old_loop_depth = tmp_loop_depth;
	    }
	    new_loop_sz += tmp_loop_sz;
	}
    }
    
    if (nr_basics > 0)
    {
	new_loop_depth = ((old_loop_depth+1) > 2) ? (old_loop_depth+1) : 2;
    }
    else
    {
	new_loop_depth = old_loop_depth + 1;
    }

    /* components of new dataloop structure:
     * - 1 dataloop
     * - (nr_basics * nr_derived) offset, el_extent, blocksize, and ptr
     * - nr_basics dataloops (for contigs)
     */
    new_loop_sz += sizeof(MPID_Dataloop) +
	((nr_basics + nr_derived) *
	 (sizeof(int) + 2*sizeof(DLOOP_Offset) + sizeof(DLOOP_Dataloop *))) +
	(nr_basics * sizeof(DLOOP_Dataloop));

    new_dlp = (DLOOP_Dataloop *)
	PREPEND_PREFIX(Dataloop_alloc)(new_loop_sz);

    /* --BEGIN ERROR HANDLING-- */
    if (!new_dlp)
    {
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */


    new_dlp->kind = DLOOP_KIND_STRUCT;
    new_dlp->el_size = -1; /* not valid for struct */
    new_dlp->el_extent = -1; /* not valid for struct; see el_extent_array */
    new_dlp->el_type = MPI_DATATYPE_NULL; /* not valid for struct */

    new_dlp->loop_params.s_t.count = nr_basics + nr_derived;

    /* TODO: ALIGNMENT!!! */
    new_dlp->loop_params.s_t.dataloop_array = (DLOOP_Dataloop **)
	(((char *) new_dlp) + sizeof(MPID_Dataloop));
    new_dlp->loop_params.s_t.blocksize_array = (int *)
	(((char *) new_dlp->loop_params.s_t.dataloop_array) +
	 (nr_basics + nr_derived) * sizeof(DLOOP_Dataloop *));
    new_dlp->loop_params.s_t.offset_array = (DLOOP_Offset *)
	(((char *) new_dlp->loop_params.s_t.blocksize_array) +
	 (nr_basics + nr_derived) * sizeof(int));
    new_dlp->loop_params.s_t.el_extent_array = (DLOOP_Offset *)
	(((char *) new_dlp->loop_params.s_t.offset_array) +
	 (nr_basics + nr_derived) * sizeof(DLOOP_Offset));

    /* position curpos for adding dataloops */
    curpos = (char *) new_dlp;
    curpos += sizeof(DLOOP_Dataloop) + (nr_basics + nr_derived) *
	(sizeof(int) + 2*sizeof(DLOOP_Offset) + sizeof(DLOOP_Dataloop *));

    for (i=0; i < count; i++)
    {
	int is_builtin;

	is_builtin = (DLOOP_Handle_hasloop_macro(oldtype_array[i])) ? 0 : 1;

	if (is_builtin)
	{
	    DLOOP_Dataloop *dummy_dlp;
	    int dummy_sz, dummy_depth;

	    /* LBs and UBs already taken care of -- skip them */
	    if (oldtype_array[i] == MPI_LB || oldtype_array[i] == MPI_UB)
	    {
		continue;
	    }

	    /* build a contig dataloop for this basic and point to that */
	    mpi_errno = PREPEND_PREFIX(Dataloop_create_contiguous)
		(blklen_array[i],
		 oldtype_array[i],
		 &dummy_dlp,
		 &dummy_sz,
		 &dummy_depth,
		 flags);

	    /* --BEGIN ERROR HANDLING-- */
	    if (mpi_errno != MPI_SUCCESS)
	    {
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */

	    PREPEND_PREFIX(Dataloop_copy)(curpos, dummy_dlp, dummy_sz);
	    new_dlp->loop_params.s_t.dataloop_array[loop_idx] =
		(DLOOP_Dataloop *) curpos;
	    curpos += dummy_sz;

	    /* we stored the block size in the contig -- use 1 here */
	    new_dlp->loop_params.s_t.blocksize_array[loop_idx] = 1;
	    new_dlp->loop_params.s_t.el_extent_array[loop_idx] =
		blklen_array[i] * dummy_dlp->el_extent;
	    PREPEND_PREFIX(Dataloop_free)(dummy_dlp);
	}
	else
	{
	    DLOOP_Dataloop *old_loop_ptr;
	    int old_loop_sz;
	    DLOOP_Offset old_extent;

	    DLOOP_Handle_get_loopptr_macro(oldtype_array[i], old_loop_ptr, 0);
	    DLOOP_Handle_get_loopsize_macro(oldtype_array[i], old_loop_sz, 0);
	    DLOOP_Handle_get_extent_macro(oldtype_array[i], old_extent);

	    PREPEND_PREFIX(Dataloop_copy)(curpos, old_loop_ptr, old_loop_sz);
	    new_dlp->loop_params.s_t.dataloop_array[loop_idx] =
		(DLOOP_Dataloop *) curpos;
	    curpos += old_loop_sz;

	    new_dlp->loop_params.s_t.blocksize_array[loop_idx] =
		blklen_array[i];
	    new_dlp->loop_params.s_t.el_extent_array[loop_idx] =
		old_extent;
	}
	new_dlp->loop_params.s_t.offset_array[loop_idx] = disp_array[i];
	loop_idx++;
    }

    *dlp_p     = new_dlp;
    *dlsz_p    = new_loop_sz;
    *dldepth_p = new_loop_depth;

    return MPI_SUCCESS;
}

/* --BEGIN ERROR HANDLING-- */
static int DLOOP_Dataloop_create_struct_memory_error(void)
{
    int mpi_errno;

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
				     MPIR_ERR_RECOVERABLE,
				     "MPID_Dataloop_create_struct",
				     __LINE__,
				     MPI_ERR_OTHER,
				     "**nomem",
				     0);
    return mpi_errno;
}
/* --END ERROR HANDLING-- */

static int DLOOP_Dataloop_create_unique_type_struct(int count,
						    int *blklen_array,
						    DLOOP_Offset *disp_array,
						    DLOOP_Type *oldtype_array,
						    int type_pos,
						    DLOOP_Dataloop **dlp_p,
						    int *dlsz_p,
						    int *dldepth_p,
						    int flags)
{
    /* the same type used more than once in the array; type_pos
     * indexes to the first of these.
     */
    int i, mpi_errno = MPI_SUCCESS, *tmp_blklen_array, cur_pos = 0;
    DLOOP_Offset *tmp_disp_array;

    /* count is an upper bound on number of type instances */
    tmp_blklen_array = (int *) MPIU_Malloc(count * sizeof(int));
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklen_array) {
	/* TODO: ??? */
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disp_array = (DLOOP_Offset *)
	MPIU_Malloc(count * sizeof(DLOOP_Offset));
    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disp_array) {
	MPIU_Free(tmp_blklen_array);
	/* TODO: ??? */
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    for (i=type_pos; i < count; i++)
    {
	if (oldtype_array[i] == oldtype_array[type_pos])
	{
	    tmp_blklen_array[cur_pos] = blklen_array[i];
	    tmp_disp_array[cur_pos]   = disp_array[i];
	    cur_pos++;
	}
    }

    mpi_errno = PREPEND_PREFIX(Dataloop_create_indexed)(cur_pos,
							tmp_blklen_array,
							tmp_disp_array,
							1, /* disp in bytes */
							oldtype_array[type_pos],
							dlp_p,
							dlsz_p,
							dldepth_p,
							flags);

    MPIU_Free(tmp_blklen_array);
    MPIU_Free(tmp_disp_array);

    return mpi_errno;

}

static int DLOOP_Dataloop_create_basic_all_bytes_struct(
	       int count,
	       int *blklen_array,
	       DLOOP_Offset *disp_array,
	       DLOOP_Type *oldtype_array,
	       DLOOP_Dataloop **dlp_p,
	       int *dlsz_p,
	       int *dldepth_p,
	       int flags)
{
    int i, mpi_errno = MPI_SUCCESS, *tmp_blklen_array, cur_pos = 0;
    DLOOP_Offset *tmp_disp_array;

    /* count is an upper bound on number of type instances */
    tmp_blklen_array = (int *) MPIU_Malloc(count * sizeof(int));

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklen_array) {
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disp_array = (DLOOP_Offset *) MPIU_Malloc(count * sizeof(DLOOP_Offset));

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disp_array) {
	MPIU_Free(tmp_blklen_array);
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    for (i=0; i < count; i++)
    {
	if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB)
	{
	    int sz = MPID_Datatype_get_basic_size(oldtype_array[i]);

	    tmp_blklen_array[cur_pos] = sz * blklen_array[i];
	    tmp_disp_array[cur_pos]   = disp_array[i];
	    cur_pos++;
	}
    }
    mpi_errno = PREPEND_PREFIX(Dataloop_create_indexed)(cur_pos,
							tmp_blklen_array,
							tmp_disp_array,
							1, /* disp in bytes */
							MPI_BYTE,
							dlp_p,
							dlsz_p,
							dldepth_p,
							flags);
    
    MPIU_Free(tmp_blklen_array);
    MPIU_Free(tmp_disp_array);

    return mpi_errno;
}

static int DLOOP_Dataloop_create_flattened_struct(int count,
						  int *blklen_array,
						  DLOOP_Offset *disp_array,
						  DLOOP_Type *oldtype_array,
						  DLOOP_Dataloop **dlp_p,
						  int *dlsz_p,
						  int *dldepth_p,
						  int flags)
{
    /* arbitrary types, convert to bytes and use indexed */
    int i, mpi_errno = MPI_SUCCESS, *tmp_blklen_array, nr_blks = 0;
    DLOOP_Offset *tmp_disp_array, bytes;
    MPID_IOV *iov_array;
    MPID_Segment *segp;

    int first_ind, last_ind;

    segp = MPID_Segment_alloc();
    /* --BEGIN ERROR HANDLING-- */
    if (!segp) {
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    /* use segment code once to count contiguous regions */
    for (i=0; i < count; i++)
    {
	int is_basic;
	    
	is_basic = (DLOOP_Handle_hasloop_macro(oldtype_array[i])) ? 0 : 1;

	if (is_basic && (oldtype_array[i] != MPI_LB &&
			 oldtype_array[i] != MPI_UB))
	{
	    nr_blks++;
	}
	else /* derived type; get a count of contig blocks */
	{
	    int tmp_nr_blks;

	    PREPEND_PREFIX(Segment_init)(NULL,
					 blklen_array[i],
					 oldtype_array[i],
					 segp,
					 0 /* homogeneous */);
	    bytes = SEGMENT_IGNORE_LAST;

	    PREPEND_PREFIX(Segment_count_contig_blocks)(segp,
							0,
							&bytes,
							&tmp_nr_blks);

	    nr_blks += tmp_nr_blks;
	}
    }

    nr_blks += 2; /* safety measure */

    iov_array = (MPID_IOV *) MPIU_Malloc(nr_blks * sizeof(MPID_IOV));
    /* --BEGIN ERROR HANDLING-- */
    if (!iov_array) {
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_blklen_array = (int *) MPIU_Malloc(nr_blks * sizeof(int));

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_blklen_array) {
	MPIU_Free(iov_array);
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    tmp_disp_array = (DLOOP_Offset *) MPIU_Malloc(nr_blks * sizeof(DLOOP_Offset));

    /* --BEGIN ERROR HANDLING-- */
    if (!tmp_disp_array) {
	MPIU_Free(iov_array);
	MPIU_Free(tmp_blklen_array);
	return DLOOP_Dataloop_create_struct_memory_error();
    }
    /* --END ERROR HANDLING-- */

    /* use segment code again to flatten the type */
    first_ind = 0;
    for (i=0; i < count; i++)
    {
	/* we're going to use the segment code to flatten the type.
	 * we put in our displacement as the buffer location, and use
	 * the blocklength as the count value to get N contiguous copies
	 * of the type.
	 *
	 * Note that we're going to get back values in bytes, so that will
	 * be our new element type.
	 */
	if (oldtype_array[i] != MPI_UB && oldtype_array[i] != MPI_LB)
	{
	    PREPEND_PREFIX(Segment_init)((char *) disp_array[i],
					 blklen_array[i],
					 oldtype_array[i],
					 segp,
					 0 /* homogeneous */);
	    
	    last_ind = nr_blks - first_ind;
	    bytes = SEGMENT_IGNORE_LAST;
	    PREPEND_PREFIX(Segment_pack_vector)(segp,
						0,
						&bytes,
						&iov_array[first_ind],
						&last_ind);
	    first_ind += last_ind;
	}
    }
    nr_blks = first_ind;

#ifdef MPID_STRUCT_FLATTEN_DEBUG
    MPIU_dbg_printf("--- start of flattened type ---\n");
    for (i=0; i < nr_blks; i++) {
	MPIU_dbg_printf("a[%d] = (%d, %d)\n", i,
			iov_array[i].MPID_IOV_BUF,
			iov_array[i].MPID_IOV_LEN);
    }
    MPIU_dbg_printf("--- end of flattened type ---\n");
#endif

    for (i=0; i < nr_blks; i++)
    {
	tmp_blklen_array[i]  = iov_array[i].MPID_IOV_LEN;
	tmp_disp_array[i] = (DLOOP_Offset) iov_array[i].MPID_IOV_BUF;
    }

    PREPEND_PREFIX(Segment_free)(segp);
    MPIU_Free(iov_array);

    mpi_errno = PREPEND_PREFIX(Dataloop_create_indexed)(nr_blks,
							tmp_blklen_array,
							tmp_disp_array,
							1, /* disp in bytes */
							MPI_BYTE,
							dlp_p,
							dlsz_p,
							dldepth_p,
							flags);
    
    MPIU_Free(tmp_blklen_array);
    MPIU_Free(tmp_disp_array);

    return mpi_errno;
}
