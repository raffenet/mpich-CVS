/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include "./dataloop.h"

struct PREPEND_PREFIX(contig_blocks_params) {
    DLOOP_Count  count;
    DLOOP_Offset last_loc;
};

/* MPID_Segment_contig_count_block
 *
 * Note: because bufp is just an offset, we can ignore it in our
 *       calculations of # of contig regions.
 */
static int DLOOP_Segment_contig_count_block(DLOOP_Offset *blocks_p,
					    DLOOP_Type el_type,
					    DLOOP_Offset rel_off,
					    DLOOP_Buffer bufp,
					    void *v_paramp)
{
    DLOOP_Offset size, el_size;
    struct PREPEND_PREFIX(contig_blocks_params) *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    size = *blocks_p * (DLOOP_Offset) el_size;

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("contig count block: count = %d, buf+off = %d, lastloc = %d\n",
		    (int) paramp->count,
		    (int) ((char *) bufp + rel_off),
		    (int) paramp->last_loc);
#endif

    if (paramp->count > 0 && rel_off == paramp->last_loc)
    {
	/* this region is adjacent to the last */
	paramp->last_loc += size;
    }
    else {
	/* new region */
	paramp->last_loc = rel_off + size;
	paramp->count++;
    }
    return 0;
}

/* MPID_Segment_count_contig_blocks()
 *
 * Count number of contiguous regions in segment between first and last.
 */
void PREPEND_PREFIX(Segment_count_contig_blocks)(DLOOP_Segment *segp,
						 DLOOP_Offset first,
						 DLOOP_Offset *lastp,
						 int *countp)
{
    struct PREPEND_PREFIX(contig_blocks_params) params;

    params.count    = 0;
    params.last_loc = 0;

    PREPEND_PREFIX(Segment_manipulate)(segp,
				       first,
				       lastp,
				       DLOOP_Segment_contig_count_block,
				       NULL, /* vector fn */
				       NULL, /* blkidx fn */
				       NULL, /* index fn */
				       NULL, /* size fn */
				       &params);

    *countp = params.count;
    return;
}

/*************/

/********** FUNCTIONS FOR CREATING AN IOV DESCRIBING BUFFER **********/

/* Segment_mpi_flatten
 *
 * Flattens into a set of blocklengths and displacements, as in an
 * MPI hindexed type. Note that we use appropriately-sized variables
 * in the associated params structure for this reason.
 *
 * NOTE: blocks will be in units of bytes when returned.
 *
 * WARNING: there's potential for overflow here as we convert from
 *          various types into an index of bytes.
 */
struct PREPEND_PREFIX(mpi_flatten_params) {
    int       index, length;
    MPI_Aint  last_end;
    int      *blklens;
    MPI_Aint *disps;
};

/* DLOOP_Segment_contig_mpi_flatten
 *
 */
static int DLOOP_Segment_contig_mpi_flatten(DLOOP_Offset *blocks_p,
					    DLOOP_Type el_type,
					    DLOOP_Offset rel_off,
					    void *bufp,
					    void *v_paramp)
{
    int last_idx, size;
    DLOOP_Offset el_size;
    char *last_end = NULL;
    struct PREPEND_PREFIX(mpi_flatten_params) *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    size = *blocks_p * (int) el_size;

#if 0
    MPIU_DBG_MSG_FMT(DATATYPE,VERBOSE,(MPIU_DBG_FDEST,
             "\t[contig to vec: do=%d, dp=%x, ind=%d, sz=%d, blksz=%d]\n",
		    (unsigned) rel_off,
		    (unsigned) (MPI_Aint) bufp,
		    paramp->index,
		    el_size,
		    (int) *blocks_p));
#endif
    
    last_idx = paramp->index - 1;
    if (last_idx >= 0) {
	last_end = ((char *) paramp->disps[last_idx]) +
	    paramp->blklens[last_idx];
    }

    if ((last_idx == paramp->length-1) &&
	(last_end != ((char *) bufp + rel_off)))
    {
	/* we have used up all our entries, and this region doesn't fit on
	 * the end of the last one.  setting blocks to 0 tells manipulation
	 * function that we are done (and that we didn't process any blocks).
	 */
	*blocks_p = 0;
	return 1;
    }
    else if (last_idx >= 0 && (last_end == ((char *) bufp + rel_off)))
    {
	/* add this size to the last vector rather than using up another one */
	paramp->blklens[last_idx] += size;
    }
    else {
	paramp->disps[last_idx+1]   = (MPI_Aint) ((char *) bufp + rel_off);
	paramp->blklens[last_idx+1] = size;
	paramp->index++;
    }
    return 0;
}

/* DLOOP_Segment_vector_mpi_flatten
 *
 * Input Parameters:
 * blocks_p - [inout] pointer to a count of blocks (total, for all noncontiguous pieces)
 * count    - # of noncontiguous regions
 * blksz    - size of each noncontiguous region
 * stride   - distance in bytes from start of one region to start of next
 * el_type - elemental type (e.g. MPI_INT)
 * ...
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int DLOOP_Segment_vector_mpi_flatten(DLOOP_Offset *blocks_p,
					    DLOOP_Count count,
					    DLOOP_Count blksz,
					    DLOOP_Offset stride,
					    DLOOP_Type el_type,
					    DLOOP_Offset rel_off, /* offset into buffer */
					    void *bufp, /* start of buffer */
					    void *v_paramp)
{
    int i, size, blocks_left;
    DLOOP_Offset el_size;
    struct PREPEND_PREFIX(mpi_flatten_params) *paramp = v_paramp;

    DLOOP_Handle_get_size_macro(el_type, el_size);
    blocks_left = *blocks_p;

#if 0
    MPIU_DBG_MSG_FMT(DATATYPE,VERBOSE,(MPIU_DBG_FDEST,
	     "\t[vector to vec: do=%d, dp=%x, len=%d, ind=%d, ct=%d, blksz=%d, str=%d, blks=%d]\n",
		    (unsigned) rel_off,
		    (unsigned) (MPI_Aint)bufp,
		    paramp->u.pack_vector.length,
		    paramp->u.pack_vector.index,
		    count,
		    blksz,
		    stride,
		    (int) *blocks_p));
#endif

    for (i=0; i < count && blocks_left > 0; i++) {
	int last_idx;
	char *last_end = NULL;

	if (blocks_left > blksz) {
	    size = blksz * (int) el_size;
	    blocks_left -= blksz;
	}
	else {
	    /* last pass */
	    size = blocks_left * (int) el_size;
	    blocks_left = 0;
	}

	last_idx = paramp->index - 1;
	if (last_idx >= 0) {
	    last_end = ((char *) paramp->disps[last_idx]) +
		paramp->blklens[last_idx];
	}

	if ((last_idx == paramp->length-1) &&
	    (last_end != ((char *) bufp + rel_off)))
	{
	    /* we have used up all our entries, and this one doesn't fit on
	     * the end of the last one.
	     */
	    *blocks_p -= (blocks_left + (size / (int) el_size));
#if 0
	    paramp->u.pack_vector.index++;
#endif
#ifdef MPID_SP_VERBOSE
	    MPIU_dbg_printf("\t[vector to vec exiting (1): next ind = %d, %d blocks processed.\n",
			    paramp->u.pack_vector.index,
			    (int) *blocks_p);
#endif
	    return 1;
	}
	else if (last_idx >= 0 && (last_end == ((char *) bufp + rel_off)))
	{
	    /* add this size to the last vector rather than using up new one */
	    paramp->blklens[last_idx] += size;
	}
	else {
	    paramp->disps[last_idx+1]   = (MPI_Aint) ((char *) bufp + rel_off);
	    paramp->blklens[last_idx+1] = size;
	    paramp->index++;
	}

	rel_off += stride;
    }

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[vector to vec exiting (2): next ind = %d, %d blocks processed.\n",
		    paramp->u.pack_vector.index,
		    (int) *blocks_p);
#endif

    /* if we get here then we processed ALL the blocks; don't need to update
     * blocks_p
     */

    DLOOP_Assert(blocks_left == 0);
    return 0;
}

/* MPID_Segment_mpi_flatten
 *
 * Parameters:
 * segp    - pointer to segment structure
 * first   - first byte in segment to pack
 * lastp   - in/out parameter describing last byte to pack (and afterwards
 *           the last byte _actually_ packed)
 *           NOTE: actually returns index of byte _after_ last one packed
 * blklens, disps - the usual blocklength and displacement arrays for MPI
 * lengthp - in/out parameter describing length of array (and afterwards
 *           the amount of the array that has actual data)
 */
void PREPEND_PREFIX(Segment_mpi_flatten)(DLOOP_Segment *segp,
					 DLOOP_Offset first,
					 DLOOP_Offset *lastp,
					 int *blklens,
					 MPI_Aint *disps,
					 int *lengthp)
{
    struct PREPEND_PREFIX(mpi_flatten_params) params;

    DLOOP_Assert(*lengthp > 0);

    params.index   = 0;
    params.length  = *lengthp;
    params.blklens = blklens;
    params.disps   = disps;

    PREPEND_PREFIX(Segment_manipulate)(segp,
				       first,
				       lastp, 
				       DLOOP_Segment_contig_mpi_flatten, 
				       DLOOP_Segment_vector_mpi_flatten,
				       NULL, /* blkidx fn */
				       NULL, /* index fn */
				       NULL,
				       &params);

    /* last value already handled by MPID_Segment_manipulate */
    *lengthp = params.index;
    return;
}

/* 
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
