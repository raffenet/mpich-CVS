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
    int i, all_basics = 1, all_same = 1, has_lb = 0, has_ub = 0;

    MPID_Datatype *new_dtp;
#if 0
    struct MPID_Dataloop *dlp;
    MPI_Aint *aptr;
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
	return mpi_errno;
    }

    
    if (HANDLE_GET_KIND(*oldtype_array) != HANDLE_KIND_BUILTIN) all_basics = 0;
    else if (*oldtype_array == MPI_LB) has_lb = 1;
    else if (*oldtype_array == MPI_UB) has_ub = 1;

    /* have a quick look at the types to see if there are any easy simplifications
     * that we can make.
     */
    for (i=1; i < count; i++) {
	if (HANDLE_GET_KIND(oldtype_array[i]) != HANDLE_KIND_BUILTIN) all_basics = 0;
	else if (oldtype_array[i] == MPI_LB) has_lb = 1;
	else if (oldtype_array[i] == MPI_UB) has_ub = 1;

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
	return mpi_errno;
    }

    if (all_basics && !has_lb && !has_ub) {
	/* no derived types and no ub/lb, but not all the same -- just convert
	 * everything to bytes.
	 */
	int *tmp_blocklength_array;
	
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
	return mpi_errno;
    }

    if (all_basics) {
	/* There are only basics, but there are LBs and/or UBs that we need to
	 * strip out.  So we convert to MPI_BYTEs as before, pull out the LBs and
	 * UBs, and then adjust the LB/UB as needed afterwards.
	 */
	int tmp_count = 0, *tmp_blocklength_array, found_lb = 0, found_ub = 0;
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

		tmp_blocklength_array[tmp_count]  = sz * blocklength_array[i];
		tmp_displacement_array[tmp_count] = displacement_array[i];
		tmp_count++;
	    } 
	}

	mpi_errno = MPID_Type_indexed(tmp_count,
				      tmp_blocklength_array,
				      tmp_displacement_array,
				      1, /* displacement in bytes */
				      MPI_BYTE,
				      newtype);	

	MPIU_Free(tmp_blocklength_array);
	MPIU_Free(tmp_displacement_array);

	/* deal with the LB and/or UB */
	MPID_Datatype_get_ptr(*newtype, new_dtp);
	if (has_lb) {
	    new_dtp->has_sticky_lb = 1;
	    new_dtp->lb            = lb_disp;
	}
	if (has_ub) {
	    new_dtp->has_sticky_ub = 1;
	    new_dtp->ub            = ub_disp;
	}

	new_dtp->extent = new_dtp->ub - new_dtp->lb;

	return mpi_errno;
    }

    /* derived types ... */
    assert(0);

    return MPI_SUCCESS;
}








