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
    if (max_alignsize > HAVE_MAX_STRUCT_ALIGNMENT) max_alignsize = HAVE_MAX_STRUCT_ALIGNMENT;
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

  This version relies on other MPID_Type routines in order to create the
  final type.

  The resulting type will bear little resemblance to what was passed in, and
  currently get contents/envelope would not work.
@*/

/*
 * THIS VERSION CONVERTS THE STRUCT INTO AN HINDEXED, SO IT'S ONLY GOOD FOR
 * HOMOGENOUS SYSTEMS!!!
 */
int MPID_Type_struct(int count,
		     int *blocklength_array,
		     MPI_Aint *displacement_array,
		     MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype)
{
    int mpi_errno = MPI_SUCCESS;
    int i, nr_real_types = 0, all_basics = 1, all_same = 1,
        has_lb_builtin = 0, has_ub_builtin = 0, eltype;

    MPID_Datatype *new_dtp, *old_dtp;

#ifdef MPID_STRUCT_DEBUG
    MPIDI_Datatype_printf(oldtype_array[0], 1, displacement_array[0], blocklength_array[0], 1);
    for (i=1; i < count; i++) {
	MPIDI_Datatype_printf(oldtype_array[i], 1, displacement_array[i], blocklength_array[i], 0);
    }
#endif

    if (count == 1) {
	/* simplest case: count == 1 */
	/* NOTE: this could be done with a contig with an LB I think? */
	mpi_errno = MPID_Type_indexed(count,
				      blocklength_array,
				      displacement_array,
				      1, /* displacement in bytes */
				      *oldtype_array,
				      newtype);
#ifdef MPID_STRUCT_DEBUG
	MPIDI_Datatype_printf(*newtype, 0, 0, 1, 0);
#endif
	return mpi_errno;
    }

    
    if (*oldtype_array == MPI_LB) has_lb_builtin = 1;
    else if (*oldtype_array == MPI_UB) has_ub_builtin = 1;
    else if (HANDLE_GET_KIND(*oldtype_array) != HANDLE_KIND_BUILTIN) {
	all_basics = 0;
	nr_real_types = 1;
    }
    else {
	nr_real_types = 1;
    }

    /* have a quick look at the types to see if there are any easy simplifications
     * that we can make.
     */
    for (i=1; i < count; i++) {
	if (oldtype_array[i] == MPI_LB) has_lb_builtin = 1;
	else if (oldtype_array[i] == MPI_UB) has_ub_builtin = 1;
	else if (HANDLE_GET_KIND(oldtype_array[i]) != HANDLE_KIND_BUILTIN) {
	    all_basics = 0;
	    nr_real_types++;
	}
	else /* builtin that isn't an LB or UB */ nr_real_types++;

	if (oldtype_array[i] != *oldtype_array) all_same = 0;
    }

    if (all_same) {
	/* they used the same type every time; this is just an hindexed again */
	mpi_errno = MPID_Type_indexed(count,
				      blocklength_array,
				      displacement_array,
				      1, /* displacement in bytes */
				      *oldtype_array,
				      newtype);

	/* alignsize and padding should be ok in this case */

#ifdef MPID_STRUCT_DEBUG
	MPIDI_Datatype_printf(*newtype, 0, 0, 1, 0);
#endif
	return mpi_errno;
    }
    else if (all_basics && !has_lb_builtin && !has_ub_builtin) {
	/* no derived types and no ub/lb, but not all the same -- just convert
	 * everything to bytes.
	 */
	int *tmp_blocklength_array, alignsize, epsilon;
	
	tmp_blocklength_array = (int *) MPIU_Malloc(count * sizeof(int));

	for (i=0; i < count; i++) {
	    int sz = MPID_Datatype_get_basic_size(oldtype_array[i]);

	    tmp_blocklength_array[i] = sz * blocklength_array[i];
	}

	mpi_errno = MPID_Type_indexed(count,
				      tmp_blocklength_array,
				      displacement_array,
				      1, /* displacement in bytes */
				      MPI_BYTE,
				      newtype);

	MPIU_Free(tmp_blocklength_array);

	/* account for padding */
	MPID_Datatype_get_ptr(*newtype, new_dtp);
	alignsize = MPID_Type_struct_alignsize(count, oldtype_array);
	new_dtp->alignsize = alignsize;
	epsilon = new_dtp->extent % alignsize;
	if (epsilon) {
	    new_dtp->ub += (alignsize - epsilon);
	    new_dtp->extent = new_dtp->ub - new_dtp->lb;
	}

	new_dtp->element_size = -1; /* not all the same size */

        new_dtp->eltype = MPI_DATATYPE_NULL; /* indicates different
                                                elementary types */

#ifdef MPID_STRUCT_DEBUG
	MPIDI_Datatype_printf(*newtype, 0, 0, 1, 0);
#endif
	return mpi_errno;
    } /* end of all basics w/out lb or ub case */
    /* TODO: Catch case of a single basic with an LB and/or UB; this can be done
     * with a contig followed by an adjustment of the LB and UB, which would be
     * simpler to process than the indexed that we use below.
     */
    else if (all_basics && nr_real_types == 1) {
	/* There are only basics, and only one, but there are LBs and/or UBs that
	 * we need to strip out.
	 */
	int type_idx = 0, found_lb = 0, found_ub = 0;
	MPI_Aint lb_disp = 0, ub_disp = 0;

	/* pass through all the types:
	 * - find the smallest LB
	 * - find the largest UB
	 * - find the real type
	 */
	for (i=0; i < count; i++) {
	    if (oldtype_array[i] == MPI_LB) {
		if (!found_lb) {
		    found_lb = 1;
		    lb_disp = displacement_array[i];
		}
		else if (displacement_array[i] < lb_disp) lb_disp = displacement_array[i];
	    }
	    else if (oldtype_array[i] == MPI_UB) {
		if (!found_ub) {
		    found_ub = 1;
		    ub_disp = displacement_array[i];
		}
		else if (displacement_array[i] > ub_disp) ub_disp = displacement_array[i];
	    }
	    else {
		type_idx = i;
	    } 
	}

	if (displacement_array[type_idx] == (MPI_Aint) 0) {
	    /* do this with a contig */
	    mpi_errno = MPID_Type_contiguous(blocklength_array[type_idx],
					     oldtype_array[type_idx],
					     newtype);
	}
	else {
	    /* use an indexed type */
	    mpi_errno = MPID_Type_indexed(1,
					  &blocklength_array[type_idx],
					  &displacement_array[type_idx],
					  1, /* displacement in bytes */
					  oldtype_array[type_idx],
					  newtype);
	}
	if (mpi_errno != MPI_SUCCESS) return mpi_errno;

	/* deal with the LB and/or UB */
	MPID_Datatype_get_ptr(*newtype, new_dtp);
	if (found_lb) {
	    new_dtp->has_sticky_lb = 1;
	    new_dtp->lb            = lb_disp;
	}
	if (found_ub) {
	    new_dtp->has_sticky_ub = 1;
	    new_dtp->ub            = ub_disp;
	}
	new_dtp->extent = new_dtp->ub - new_dtp->lb;

        new_dtp->eltype = oldtype_array[type_idx];

	return mpi_errno;
    }
    else if (all_basics) {
	/* There are only basics, but there are LBs and/or UBs that we need to
	 * strip out.  So we convert to MPI_BYTEs as before, pull out the LBs and
	 * UBs, and then adjust the LB/UB as needed afterwards.
	 */
	int tmp_idx = 0, *tmp_blocklength_array, found_lb = 0, found_ub = 0;
	MPI_Aint lb_disp = 0, ub_disp = 0, *tmp_displacement_array;

	/* don't bother to figure out exactly how much space we really need; use
	 * count as an upper bound instead.
	 */
	tmp_blocklength_array  = (int *) MPIU_Malloc(count * sizeof(int));
	tmp_displacement_array = (MPI_Aint *) MPIU_Malloc(count * sizeof(MPI_Aint));

	/* pass through all the types:
	 * - find the smallest LB
	 * - find the largest UB
	 * - create the tmp blocklength and displacemment arrays
	 * - get tmp count
	 */
        eltype = MPI_LB; /*initialize */
	for (i=0; i < count; i++) {
	    if (oldtype_array[i] == MPI_LB) {
		if (!found_lb) {
		    found_lb = 1;
		    lb_disp = displacement_array[i];
		}
		else if (displacement_array[i] < lb_disp) lb_disp = displacement_array[i];
	    }
	    else if (oldtype_array[i] == MPI_UB) {
		if (!found_ub) {
		    found_ub = 1;
		    ub_disp = displacement_array[i];
		}
		else if (displacement_array[i] > ub_disp) ub_disp = displacement_array[i];
	    }
	    else {
		int sz = MPID_Datatype_get_basic_size(oldtype_array[i]);

                if (eltype == MPI_LB) eltype = oldtype_array[i];
                else if (eltype != oldtype_array[i]) 
                    eltype = MPI_DATATYPE_NULL;

		tmp_blocklength_array[tmp_idx]  = sz * blocklength_array[i];
		tmp_displacement_array[tmp_idx] = displacement_array[i];
		tmp_idx++;
	    } 
	}

	mpi_errno = MPID_Type_indexed(tmp_idx,
				      tmp_blocklength_array,
				      tmp_displacement_array,
				      1, /* displacement in bytes */
				      MPI_BYTE,
				      newtype);	

	MPIU_Free(tmp_blocklength_array);
	MPIU_Free(tmp_displacement_array);

	if (mpi_errno != MPI_SUCCESS) return mpi_errno;

	/* deal with the LB and/or UB */
	MPID_Datatype_get_ptr(*newtype, new_dtp);
	if (found_lb) {
	    new_dtp->has_sticky_lb = 1;
	    new_dtp->lb            = lb_disp;
	}
	if (found_ub) {
	    new_dtp->has_sticky_ub = 1;
	    new_dtp->ub            = ub_disp;
	}
	new_dtp->extent = new_dtp->ub - new_dtp->lb;

        new_dtp->eltype = eltype;

	/* TODO: don't bother with alignsize because of the lb/ub (?) */

#ifdef MPID_STRUCT_DEBUG
	MPIDI_Datatype_printf(*newtype, 0, 0, 1, 0);
#endif
	return mpi_errno;
    } /* end of all basics w/ lb and/or ub case */
    else if (nr_real_types == 1) {
	/* There's exactly one derived type in the struct.
	 * 
	 * steps:
	 * - find the locations of the UB and LB, index of actual type
	 * - ...
	 *
	 * NOTE: if displacement of type is zero, we can dup and adjust
	 * lb/ub/extent.  otherwise we need to deal with the displacement.
	 * the easiest way to do that is with an byte indexed type again.
	 */
	int found_lb = 0, found_ub = 0, real_type_idx = -1;
	MPI_Aint lb_disp = -1, ub_disp = -1;

	for (i=0; i < count; i++) {
	    if (oldtype_array[i] == MPI_LB) {
		if (!found_lb) {
		    found_lb = 1;
		    lb_disp = displacement_array[i];
		}
		else if (displacement_array[i] < lb_disp) lb_disp = displacement_array[i];
	    }
	    else if (oldtype_array[i] == MPI_UB) {
		if (!found_ub) {
		    found_ub = 1;
		    ub_disp = displacement_array[i];
		}
		else if (displacement_array[i] > ub_disp) ub_disp = displacement_array[i];
	    }
	    else real_type_idx = i;
	}

	if (displacement_array[real_type_idx] == 0 && blocklength_array[real_type_idx] == 1) {
	    mpi_errno = MPID_Type_dup(oldtype_array[real_type_idx],
				      newtype);
	}
	else if (displacement_array[real_type_idx] == 0) {
	    mpi_errno = MPID_Type_contiguous(blocklength_array[real_type_idx],
					     oldtype_array[real_type_idx],
					     newtype);
	}
	else {
	    mpi_errno = MPID_Type_indexed(1,
					  &blocklength_array[real_type_idx],
					  &displacement_array[real_type_idx],
					  1,
					  oldtype_array[real_type_idx],
					  newtype);
	}
	if (mpi_errno != MPI_SUCCESS) return mpi_errno;

	MPID_Datatype_get_ptr(*newtype, new_dtp);
	MPID_Datatype_get_ptr(oldtype_array[real_type_idx], old_dtp);

	new_dtp->is_committed = 0; /* especially for dup'd type */
	if (found_lb) {
	    new_dtp->has_sticky_lb = 1;
	    if (old_dtp->has_sticky_lb && old_dtp->lb < lb_disp) lb_disp = old_dtp->lb;
	    new_dtp->lb            = lb_disp;
	}
	if (found_ub) {
	    new_dtp->has_sticky_ub = 1;
	    if (old_dtp->has_sticky_ub && old_dtp->ub > ub_disp) ub_disp = old_dtp->ub;
	    new_dtp->ub            = ub_disp;
	}
	new_dtp->extent = new_dtp->ub - new_dtp->lb;
	
	/* alignsize and padding should be ok in this case */

#ifdef MPID_STRUCT_DEBUG
	MPIDI_Datatype_printf(*newtype, 0, 0, 1, 0);
#endif
	return mpi_errno;
    } /* end of single derived type case */
    else {
    	/* note: first and last are used for indices in this code, not byte positions */
	int nr_pieces = 0, first = 0, last, found_lb = 0, found_ub = 0, epsilon, alignsize;
	int *tmp_blocklength_array;
	MPI_Aint *tmp_displacement_array, bytes, lb_disp = 0, ub_disp = 0;
	MPID_IOV *iov_array;
	MPID_Segment *segp;

	/* General case: more than one type, not all of which are basics.
	 *
	 * We're going to flatten the whole thing into a collection 
	 * of contiguous pieces and make an hindexed out of that.
	 *
	 * Three passes (roughly):
	 * - First figure out how many pieces there are going to be.
	 * - Allocate and fill in arrays of extents for type.
	 * - Recalculate the number of elements (basics)
	 * 
	 */

        eltype = MPI_LB; 
	for (i=0; i < count; i++) {
	    int tmp_pieces = 0, tmp_lb, tmp_ub;

	    if (oldtype_array[i] == MPI_LB) {
		if (!found_lb) {
		    found_lb = 1;
		    lb_disp = displacement_array[i];
		}
		else if (displacement_array[i] < lb_disp) lb_disp = displacement_array[i];
	    }
	    else if (oldtype_array[i] == MPI_UB) {
		if (!found_ub) {
		    found_ub = 1;
		    ub_disp = displacement_array[i];
		}
		else if (displacement_array[i] > ub_disp) ub_disp = displacement_array[i];
	    }
	    else if (HANDLE_GET_KIND(oldtype_array[i]) ==
                     HANDLE_KIND_BUILTIN) {
                tmp_pieces = 1;
                if (eltype == MPI_LB) eltype = oldtype_array[i];
                else if (eltype != oldtype_array[i])
                    eltype = MPI_DATATYPE_NULL;
            }
	    else {
		MPID_Datatype_get_ptr(oldtype_array[i], old_dtp);
                if (eltype == MPI_LB) eltype = old_dtp->eltype;
                else if (eltype != old_dtp->eltype)
                    eltype = MPI_DATATYPE_NULL;

		/* calculate lb and ub of this type; see mpid_datatype.h */
		MPID_DATATYPE_BLOCK_LB_UB(blocklength_array[i],
					  displacement_array[i],
					  old_dtp->lb,
					  old_dtp->ub,
					  old_dtp->extent,
					  tmp_lb,
					  tmp_ub);

		tmp_pieces = old_dtp->n_elements;
		if (old_dtp->has_sticky_lb) {
		    if (!found_lb) {
			found_lb = 1;
			lb_disp = tmp_lb;
		    }
		    else if (tmp_lb < lb_disp)
			lb_disp = tmp_lb;
		}
		if (old_dtp->has_sticky_ub) {
		    if (!found_ub) {
			found_ub = 1;
			ub_disp = tmp_ub;
		    }
		    else if (tmp_ub > ub_disp)
			ub_disp = tmp_ub;
		}
	    }
	    /* nr_pieces is an upper bound only; for example, a contig will only
	     * take up one slot regardless of the blocklength
	     */
	    nr_pieces += tmp_pieces * blocklength_array[i];
	}

	nr_pieces += 2; /* TEMPORARY SANITY CHECK!!! */

	iov_array = (MPID_IOV *) MPIU_Malloc(nr_pieces * sizeof(MPID_IOV));
	if (iov_array == NULL) assert(0);
	tmp_blocklength_array = (int *) MPIU_Malloc(nr_pieces * sizeof(int));
	if (tmp_blocklength_array == NULL) assert(0);
	tmp_displacement_array = (MPI_Aint *) MPIU_Malloc(nr_pieces * sizeof(MPI_Aint));
	if (tmp_displacement_array == NULL) assert(0);

	segp = MPID_Segment_alloc();

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
		MPID_Segment_init((char *) displacement_array[i],
				  blocklength_array[i],
				  oldtype_array[i],
				  segp);
	    
		last  = nr_pieces - first;
		bytes = INT_MAX;
		/* TODO: CREATE MANIPULATION ROUTINES THAT TAKE THE LEN AND DISP
		 * ARRAYS AND FILL THEM IN DIRECTLY.
		 */
		MPID_Segment_pack_vector(segp,
					 0,
					 &bytes, /* don't care, just want it to go */
					 &iov_array[first],
					 &last);
		first += last;
	    }
	}
	nr_pieces = first;

#ifdef MPID_STRUCT_FLATTEN_DEBUG
	MPIU_dbg_printf("--- start of flattened type ---\n");
	for (i=0; i < nr_pieces; i++) {
	    MPIU_dbg_printf("a[%d] = (%d, %d)\n", i,
			    iov_array[i].MPID_IOV_BUF,
			    iov_array[i].MPID_IOV_LEN);
	}
	MPIU_dbg_printf("--- end of flattened type ---\n");
#endif

	for (i=0; i < nr_pieces; i++) {
	    tmp_blocklength_array[i]  = iov_array[i].MPID_IOV_LEN;
	    tmp_displacement_array[i] = (MPI_Aint) iov_array[i].MPID_IOV_BUF;
	}

	MPID_Segment_free(segp);
	MPIU_Free(iov_array);

	mpi_errno = MPID_Type_indexed(nr_pieces,
				      tmp_blocklength_array,
				      tmp_displacement_array,
				      1,
				      MPI_BYTE,
				      newtype);
	if (mpi_errno != MPI_SUCCESS) assert(0);

	MPIU_Free(tmp_displacement_array);
	MPIU_Free(tmp_blocklength_array);

	MPID_Datatype_get_ptr(*newtype, new_dtp);
	if (found_lb) {
	    new_dtp->has_sticky_lb = 1;
	    new_dtp->lb            = lb_disp;
	}
	if (found_ub) {
	    new_dtp->has_sticky_ub = 1;
	    new_dtp->ub            = ub_disp;
	}
	new_dtp->extent = new_dtp->ub - new_dtp->lb;

	if (!found_lb && !found_ub) {
	    /* account for padding */
	    MPID_Datatype_get_ptr(*newtype, new_dtp);
	    alignsize = MPID_Type_struct_alignsize(count, oldtype_array);
	    new_dtp->alignsize = alignsize;
	    epsilon = new_dtp->extent % alignsize;
	    if (epsilon) {
		new_dtp->ub += (alignsize - epsilon);
		new_dtp->extent = new_dtp->ub - new_dtp->lb;
	    }
	}

	new_dtp->element_size = -1;
        new_dtp->eltype = eltype;

#ifdef MPID_STRUCT_DEBUG
	MPIDI_Datatype_printf(*newtype, 0, 0, 1, 0);
#endif
	return mpi_errno;
    } /* end of general case */
}


