/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#undef MPID_STRUCT_FLATTEN_DEBUG
#undef MPID_STRUCT_DEBUG

int MPID_Type_struct_alignsize(int count,
			       MPI_Datatype *oldtype_array);

int MPID_Type_struct_alignsize(int count,
			       MPI_Datatype *oldtype_array)
{
    int i, max_alignsize = 0, tmp_alignsize;

    for (i=0; i < count; i++) {
	/* shouldn't be called with an LB or UB, but we'll handle it nicely */
	if (oldtype_array[i] == MPI_LB || oldtype_array[i] == MPI_UB) continue;
	else if (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN) {
	    tmp_alignsize = MPID_Datatype_get_basic_size(oldtype_array[i]);
	}
	else {
	    MPID_Datatype *dtp;	    

	    MPID_Datatype_get_ptr(oldtype_array[i], dtp);
	    tmp_alignsize = dtp->alignsize;
	}
	if (max_alignsize < tmp_alignsize) max_alignsize = tmp_alignsize;
    }

#ifdef HAVE_MAX_STRUCT_ALIGNMENT
    if (max_alignsize > HAVE_MAX_STRUCT_ALIGNMENT)
	max_alignsize = HAVE_MAX_STRUCT_ALIGNMENT;
#endif
    /* if we didn't calculate a maximum struct alignment (above), then the
     * alignment was either "largest", in which case we just use what we found,
     * or "unknown", in which case what we found is as good a guess as any.
     */

    return max_alignsize;
}


/*@
  MPID_Type_struct - create a struct datatype
 
  Input Parameters:
+ count - number of blocks in vector
. blocklength_array - number of elements in each block
. displacement_array - offsets of blocks from start of type in bytes
- oldtype_array - types (using handle) of datatypes on which vector is based

  Output Parameters:
. newtype - handle of new struct datatype

  Return Value:
  0 on success, -1 on failure.
@*/
int MPID_Type_struct(int count,
		     int *blocklength_array,
		     MPI_Aint *displacement_array,
		     MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, old_are_contig = 1;
    int found_sticky_lb = 0, found_sticky_ub = 0;
    int el_sz = 0, size = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    MPI_Aint true_lb_disp = 0, true_ub_disp = 0, sticky_lb_disp = 0,
	sticky_ub_disp = 0;

    MPID_Datatype *new_dtp;

#ifdef MPID_STRUCT_DEBUG
    MPIDI_Datatype_printf(oldtype_array[0], 1, displacement_array[0],
			  blocklength_array[0], 1);
    for (i=1; i < count; i++) {
	MPIDI_Datatype_printf(oldtype_array[i], 1, displacement_array[i],
			      blocklength_array[i], 0);
    }
#endif

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					 "MPID_Type_struct",
					 __LINE__, MPI_ERR_OTHER,
					 "**nomem", 0);
	return mpi_errno;
    }

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

    for (i=0; i < count; i++) {
	int is_builtin =
	    (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN);
	MPI_Aint tmp_lb, tmp_ub, tmp_true_lb, tmp_true_ub;
	int tmp_el_sz;
	MPI_Datatype tmp_el_type;
	MPID_Datatype *old_dtp = NULL;

	if (is_builtin) {
	    /* Q: DO LB or UBs count in element counts? */
	    tmp_el_sz   = MPID_Datatype_get_basic_size(oldtype_array[i]);
	    tmp_el_type = oldtype_array[i];

	    tmp_lb      = displacement_array[i];
	    tmp_ub      = displacement_array[i] + tmp_el_sz;
	    tmp_true_lb = tmp_lb;
	    tmp_true_ub = tmp_ub;

	    size += tmp_el_sz * blocklength_array[i];
	}
	else {
	    MPID_Datatype_get_ptr(oldtype_array[i], old_dtp);

	    tmp_el_sz   = old_dtp->element_size;
	    tmp_el_type = old_dtp->eltype;

	    MPID_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
				      displacement_array[i],
				      old_dtp->lb,
				      old_dtp->ub,
				      old_dtp->extent,
				      tmp_lb,
				      tmp_ub);
	    MPID_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
				      displacement_array[i],
				      old_dtp->true_lb,
				      old_dtp->true_ub,
				      old_dtp->extent,
				      tmp_true_lb,
				      tmp_true_ub);

	    size += old_dtp->size * blocklength_array[i];
	}

	/* element size and type */
	if (i == 0) {
	    el_sz = tmp_el_sz;
	    el_type = tmp_el_type;
	}
	else if (el_sz != tmp_el_sz) {
	    /* Q: should LB and UB have any effect here? */
	    el_sz = -1;
	    el_type = MPI_DATATYPE_NULL;
	}
	else if (el_type != tmp_el_type) {
	    /* Q: should we set el_sz = -1 even though the same? */
	    el_type = MPI_DATATYPE_NULL;
	}

	/* keep up with sticky lb and ub separately */
	if ((oldtype_array[i] == MPI_LB) ||
	    (!is_builtin && old_dtp->has_sticky_lb))
	{
	    if (!found_sticky_lb) {
		found_sticky_lb = 1;
		sticky_lb_disp = tmp_lb;
	    }
	    else if (sticky_lb_disp > tmp_lb) {
		sticky_lb_disp = tmp_lb;
	    }
	}
	else if ((oldtype_array[i] == MPI_UB) || 
	    (!is_builtin && old_dtp->has_sticky_ub))
	{
	    if (!found_sticky_ub) {
		found_sticky_ub = 1;
		sticky_ub_disp = tmp_ub;
	    }
	    else if (sticky_ub_disp < tmp_ub) {
		sticky_ub_disp = tmp_ub;
	    }
	}

	/* save lowest true lb and highest true ub */
	if ((i == 0) || (true_lb_disp > tmp_true_lb))
	{
	    true_lb_disp = tmp_true_lb;
	}
	if ((i == 0) || (true_ub_disp < tmp_true_ub))
	{
	    true_ub_disp = tmp_true_ub;
	}

	if (!is_builtin && !old_dtp->is_contig) {
	    old_are_contig = 0;
	}
    }

    new_dtp->n_elements = -1; /* TODO */
    new_dtp->element_size = el_sz;
    new_dtp->eltype = el_type;

    new_dtp->has_sticky_lb = found_sticky_lb;
    new_dtp->true_lb       = true_lb_disp;
    new_dtp->lb = (found_sticky_lb) ? sticky_lb_disp : true_lb_disp;

    new_dtp->has_sticky_ub = found_sticky_ub;
    new_dtp->true_ub       = true_ub_disp;
    new_dtp->ub = (found_sticky_ub) ? sticky_ub_disp : true_ub_disp;

    new_dtp->alignsize = MPID_Type_struct_alignsize(count, oldtype_array);

    new_dtp->extent = new_dtp->ub - new_dtp->lb;
    if ((!found_sticky_lb) && (!found_sticky_ub)) {
	/* account for padding */
	MPI_Aint epsilon = new_dtp->extent % new_dtp->alignsize;

	if (epsilon) {
	    new_dtp->ub    += (new_dtp->alignsize - epsilon);
	    new_dtp->extent = new_dtp->ub - new_dtp->lb;
	}
    }

    new_dtp->size = size;

    /* new type is contig for N types if its size and extent are the
     * same, and the old type was also contiguous
     */
    if ((new_dtp->size == new_dtp->extent) && old_are_contig) {
	new_dtp->is_contig = 1;
    }
    else {
	new_dtp->is_contig = 0;
    }

    /* fill in dataloop */
    MPID_Dataloop_create_struct(count,
				blocklength_array,
				displacement_array,
				oldtype_array,
				&(new_dtp->loopinfo),
				&(new_dtp->loopsize),
				&(new_dtp->loopinfo_depth),
				MPID_DATALOOP_HOMOGENEOUS);

    *newtype = new_dtp->handle;

    return MPI_SUCCESS;
}

/*@
  MPID_Dataloop_create_struct - create the dataloop representation for a
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
  None
@*/
void MPID_Dataloop_create_struct(int count,
				 int *blklen_array,
				 MPI_Aint *disp_array,
				 MPI_Datatype *oldtype_array,
				 MPID_Dataloop **dlp_p,
				 int *dlsz_p,
				 int *dldepth_p,
				 int flags)
{
    int i, nr_basics = 0, nr_derived = 0, type_pos = 0;

    MPI_Datatype first_basic = MPI_DATATYPE_NULL,
	first_derived = MPI_DATATYPE_NULL;

    /* browse the old types and characterize */
    for (i=0; i < count; i++) {
	if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB)
	{
	    if (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN) {
		if (nr_basics == 0) {
		    first_basic = oldtype_array[i];
		    type_pos = i;
		}
		else if (oldtype_array[i] != first_basic) {
		    first_basic = MPI_DATATYPE_NULL;
		}
		nr_basics++;
	    }
	    else /* derived type */ {
		if (nr_derived == 0) {
		    first_derived = oldtype_array[i];
		    type_pos = i;
		}
		else if (oldtype_array[i] != first_derived) {
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

    if (nr_basics + nr_derived == 0) {
	/* WHAT DO WE DO HERE? */
	assert(0);
    }
    else if (nr_basics + nr_derived == 1) {
	/* type_pos is index to only real type in array */
	/* note: indexed call will a contig if possible */
	MPID_Dataloop_create_indexed(1, /* count */
				     &blklen_array[type_pos],
				     &disp_array[type_pos],
				     1, /* displacement in bytes */
				     oldtype_array[type_pos],
				     dlp_p,
				     dlsz_p,
				     dldepth_p,
				     flags);
	return;
    }
    else if (((nr_derived == 0) && (first_basic != MPI_DATATYPE_NULL)) ||
	     ((nr_basics == 0) && (first_derived != MPI_DATATYPE_NULL)))
    {
	/* the same type used more than once in the array; type_pos
	 * indexes to the first of these.
	 */
	int *tmp_blklen_array, cur_pos = 0;
	MPI_Aint *tmp_disp_array;

	/* count is an upper bound on number of type instances */
	tmp_blklen_array = (int *) MPIU_Malloc(count * sizeof(int));
	assert(tmp_blklen_array != NULL);
	tmp_disp_array   = (MPI_Aint *) MPIU_Malloc(count * sizeof(MPI_Aint));
	assert(tmp_disp_array != NULL);

	for (i=type_pos; i < count; i++) {
	    if (oldtype_array[i] == oldtype_array[type_pos]) {
		tmp_blklen_array[cur_pos] = blklen_array[i];
		tmp_disp_array[cur_pos]   = disp_array[i];
		cur_pos++;
	    }
	}

	MPID_Dataloop_create_indexed(cur_pos,
				     tmp_blklen_array,
				     tmp_disp_array,
				     1, /* displacement in bytes */
				     oldtype_array[type_pos],
				     dlp_p,
				     dlsz_p,
				     dldepth_p,
				     flags);
	MPIU_Free(tmp_blklen_array);
	MPIU_Free(tmp_disp_array);
	return;
    }
    else if (nr_derived == 0 && ((flags & MPID_DATALOOP_HOMOGENEOUS) ||
				 (flags & MPID_DATALOOP_ALL_BYTES)))
    {
	/* all basics, convert to bytes and use indexed */
	int *tmp_blklen_array, cur_pos = 0;
	MPI_Aint *tmp_disp_array;

	/* count is an upper bound on number of type instances */
	tmp_blklen_array = (int *) MPIU_Malloc(count * sizeof(int));
	assert(tmp_blklen_array != NULL);
	tmp_disp_array   = (MPI_Aint *) MPIU_Malloc(count * sizeof(MPI_Aint));
	assert(tmp_disp_array != NULL);

	for (i=0; i < count; i++) {
	    if (oldtype_array[i] != MPI_LB && oldtype_array[i] != MPI_UB) {
		int sz = MPID_Datatype_get_basic_size(oldtype_array[i]);

		tmp_blklen_array[cur_pos] = sz * blklen_array[i];
		tmp_disp_array[cur_pos]   = disp_array[i];
		cur_pos++;
	    }
	}
	MPID_Dataloop_create_indexed(cur_pos,
				     tmp_blklen_array,
				     tmp_disp_array,
				     1, /* displacement in bytes */
				     MPI_BYTE,
				     dlp_p,
				     dlsz_p,
				     dldepth_p,
				     flags);
	MPIU_Free(tmp_blklen_array);
	MPIU_Free(tmp_disp_array);
	return;
    }
    else if ((flags & MPID_DATALOOP_HOMOGENEOUS) ||
	     (flags & MPID_DATALOOP_ALL_BYTES))
    {
	/* arbitrary types, convert to bytes and use indexed */
	int *tmp_blklen_array, nr_blks = 0;
	MPI_Aint *tmp_disp_array;
	MPID_IOV *iov_array;
	MPID_Segment *segp;

	MPI_Aint first, last, bytes;

	segp = MPID_Segment_alloc();

	/* use segment code once to count contiguous regions */
	for (i=0; i < count; i++) {
	    int is_basic;
	    
	    is_basic = (HANDLE_GET_KIND(oldtype_array[i]) == HANDLE_KIND_BUILTIN);

	    if (is_basic && (oldtype_array[i] != MPI_LB &&
			     oldtype_array[i] != MPI_UB))
	    {
		nr_blks++;
	    }
	    else /* derived type; get a count of contig blocks */ {
		int tmp_nr_blks;

		MPID_Segment_init(NULL,
				  blklen_array[i],
				  oldtype_array[i],
				  segp);
		bytes = SEGMENT_IGNORE_LAST;

		MPID_Segment_count_contig_blocks(segp,
						 0,
						 &bytes,
						 &tmp_nr_blks);

		nr_blks += tmp_nr_blks;
	    }
	}

	nr_blks += 2; /* safety measure */

	iov_array = (MPID_IOV *) MPIU_Malloc(nr_blks * sizeof(MPID_IOV));
	assert(iov_array != NULL);
	tmp_blklen_array = (int *) MPIU_Malloc(nr_blks * sizeof(int));
	assert(tmp_blklen_array != NULL);
	tmp_disp_array = (MPI_Aint *) MPIU_Malloc(nr_blks * sizeof(MPI_Aint));
	assert(tmp_disp_array != NULL);

	/* use segment code again to flatten the type */
	first = 0;
	for (i=0; i < count; i++) {
	    /* we're going to use the segment code to flatten the type.
	     * we put in our displacement as the buffer location, and use
	     * the blocklength as the count value to get N contiguous copies
	     * of the type.
	     *
	     * Note that we're going to get back values in bytes, so that will
	     * be our new element type.
	     */
	    if (oldtype_array[i] != MPI_UB && oldtype_array[i] != MPI_LB) {
		MPID_Segment_init((char *) disp_array[i],
				  blklen_array[i],
				  oldtype_array[i],
				  segp);
	    
		last  = nr_blks - first;
		bytes = SEGMENT_IGNORE_LAST;
		MPID_Segment_pack_vector(segp,
					 0,
					 &bytes,
					 &iov_array[first],
					 &last);
		first += last;
	    }
	}
	nr_blks = first;

#ifdef MPID_STRUCT_FLATTEN_DEBUG
	MPIU_dbg_printf("--- start of flattened type ---\n");
	for (i=0; i < nr_blks; i++) {
	    MPIU_dbg_printf("a[%d] = (%d, %d)\n", i,
			    iov_array[i].MPID_IOV_BUF,
			    iov_array[i].MPID_IOV_LEN);
	}
	MPIU_dbg_printf("--- end of flattened type ---\n");
#endif

	for (i=0; i < nr_blks; i++) {
	    tmp_blklen_array[i]  = iov_array[i].MPID_IOV_LEN;
	    tmp_disp_array[i] = (MPI_Aint) iov_array[i].MPID_IOV_BUF;
	}

	MPID_Segment_free(segp);
	MPIU_Free(iov_array);

	MPID_Dataloop_create_indexed(nr_blks,
				     tmp_blklen_array,
				     tmp_disp_array,
				     1, /* displacement in bytes */
				     MPI_BYTE,
				     dlp_p,
				     dlsz_p,
				     dldepth_p,
				     flags);
	MPIU_Free(tmp_blklen_array);
	MPIU_Free(tmp_disp_array);
	return;
    }
    else /* general case, allow branches */ {
	assert(0);
	return;
    }
}
