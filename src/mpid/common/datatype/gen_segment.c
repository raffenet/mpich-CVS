/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Includes specific to MPICH2 use of this code. */
#include <mpid_dataloop.h>
#include <mpiimpl.h>

#undef DEBUG_DLOOP_MANIPULATE

#ifndef GEN_DATALOOP_H
#error "You must explicitly include a header that sets the PREPEND_PREFIX and includes gen_dataloop.h"
#endif

/* Notes on functions:
 *
 * There are a few different sets of functions here:
 * - DLOOP_Segment_manipulate() - uses a "piece" function to perform operations
 *   using segments (piece functions defined elsewhere)
 * - PREPEND_PREFIX functions - these define the externally visible interface
 *   to segment functionality
 */

static inline int DLOOP_Stackelm_blocksize(struct DLOOP_Dataloop_stackelm *elmp);
static inline int DLOOP_Stackelm_offset(struct DLOOP_Dataloop_stackelm *elmp);

/* Segment_init
 *
 * buf    - datatype buffer location
 * count  - number of instances of the datatype in the buffer
 * handle - handle for datatype (could be derived or not)
 * segp   - pointer to previously allocated segment structure
 *
 * Assumes that the segment has been allocated.
 *
 * NOTE: THIS IMPLEMENTATION DOES NOT HANDLE STRUCT DATALOOPS.
 */
int PREPEND_PREFIX(Segment_init)(const DLOOP_Buffer buf,
				 DLOOP_Count count,
				 DLOOP_Handle handle, 
				 struct DLOOP_Segment *segp)
{
    int i, elmsize = 0, depth = 0;
    struct DLOOP_Dataloop_stackelm *elmp;
    struct DLOOP_Dataloop *dlp = 0, *sblp = &segp->builtin_loop;
    
#ifdef DEBUG_DLOOP_MANIPULATE
    DLOOP_dbg_printf("DLOOP_Segment_init: count = %d, buf = %x\n",
		    count,
		    buf);
#endif

    if (!DLOOP_Handle_hasloop_macro(handle)) {
	/* simplest case; datatype has no loop (basic) */

	DLOOP_Handle_get_size_macro(handle, elmsize);

	sblp->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
	sblp->handle = handle;
	sblp->loop_params.c_t.count = count;
	sblp->loop_params.c_t.dataloop = 0;
	sblp->el_size = elmsize;
        DLOOP_Handle_get_basic_type_macro(handle, sblp->el_type);
	DLOOP_Handle_get_extent_macro(handle, sblp->el_extent);

	dlp = sblp;
	depth = 1;
    }
    else if (count == 0) {
	/* only use the builtin, call it 0 ints */
	sblp->kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
	sblp->handle = MPI_INT;
	sblp->loop_params.c_t.count = 0;
	sblp->loop_params.c_t.dataloop = 0;
	sblp->el_size = 0;
	sblp->el_extent = 0;

	dlp = sblp;
	depth = 1;
    }
    else if (count == 1) {
	/* don't use the builtin */
	DLOOP_Handle_get_loopptr_macro(handle, dlp);
	DLOOP_Handle_get_loopdepth_macro(handle, depth);
    }
    else {
	/* default: need to use builtin to handle contig; must check
	 * loop depth first
	 */
	DLOOP_Dataloop *oldloop; /* loop from original type, before new count */
	DLOOP_Offset type_size, type_extent;
	DLOOP_Type el_type;
	
	DLOOP_Handle_get_loopdepth_macro(handle, depth);
	if (depth >= DLOOP_MAX_DATATYPE_DEPTH) return -1;

	DLOOP_Handle_get_loopptr_macro(handle, oldloop);
	DLOOP_Handle_get_size_macro(handle, type_size);
	DLOOP_Handle_get_extent_macro(handle, type_extent);
        DLOOP_Handle_get_basic_type_macro(handle, el_type);

	if (depth == 1 && ((oldloop->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG))
	{
	    if (type_size == type_extent)
	    {
		/* use a contig */
		sblp->kind                     = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK;
		sblp->loop_params.c_t.count    = count * oldloop->loop_params.c_t.count;
		sblp->loop_params.c_t.dataloop = NULL;
		sblp->el_size                  = oldloop->el_size;
		sblp->el_extent                = oldloop->el_extent;
		sblp->el_type                  = oldloop->el_type;
	    }
	    else
	    {
		/* use a vector, with extent of original type becoming the stride */
		sblp->kind                      = DLOOP_KIND_VECTOR | DLOOP_FINAL_MASK;
		sblp->loop_params.v_t.count     = count;
		sblp->loop_params.v_t.blocksize = oldloop->loop_params.c_t.count;
		sblp->loop_params.v_t.stride    = type_extent;
		sblp->loop_params.v_t.dataloop  = NULL;
		sblp->el_size                   = oldloop->el_size;
		sblp->el_extent                 = oldloop->el_extent;
		sblp->el_type                   = oldloop->el_type;
	    }
	}
	else
	{
	    /* general case */
	    sblp->kind                     = DLOOP_KIND_CONTIG;
	    sblp->loop_params.c_t.count    = count;
	    sblp->loop_params.c_t.dataloop = oldloop;
	    sblp->el_size                  = type_size;
	    sblp->el_extent                = type_extent;
	    sblp->el_type                  = el_type;

	    depth++; /* we're adding to the depth with the builtin */
	}

	dlp = sblp;
    }

    /* initialize the rest of the segment values */
    segp->handle = handle;
    segp->ptr = (DLOOP_Buffer) buf;
    segp->stream_off = 0;
    segp->cur_sp = 0;
    segp->valid_sp = 0;

    /* initialize the first stackelm in its entirety */
    /* note: counts and blocks start at maximum value and are decremented to 0 */
    elmp = &(segp->stackelm[0]);
    elmp->loop_p = dlp;

    /* note: for consistency we use blksize to hold the size of a contig, not count */
    if ((dlp->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG) {
	elmp->orig_count = 1;
    }
    else {
	elmp->orig_count = dlp->loop_params.count;
    }
    
    elmp->curcount    = elmp->orig_count;
    elmp->orig_offset = 0;

    /* note: DLOOP_Stackelm_blocksize assumes orig_count, curcount, and loop_p are correct */
    elmp->orig_block  = DLOOP_Stackelm_blocksize(elmp);
    elmp->curblock    = elmp->orig_block;

    /* note: DLOOP_Stackelm_offset assumes orig_count, curcount, and loop_p are correct */
    elmp->curoffset   = /* elmp->orig_offset + */ DLOOP_Stackelm_offset(elmp);
    
    dlp = dlp->loop_params.cm_t.dataloop;

    for (i=1; i < depth; i++) {
	/* loop_p, orig_count, orig_block, and curcount are all filled by us now.
	 * the rest are filled in at processing time.
	 */
	elmp = &(segp->stackelm[i]);
	elmp->loop_p     = dlp; /* DO NOT MOVE THIS BELOW THE Stackelm CALLS! */
	if ((dlp->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG) {
	    elmp->orig_count = 1;
	}
	else {
	    elmp->orig_count = dlp->loop_params.count;
	}
	elmp->curcount   = elmp->orig_count; /* required by DLOOP_Stackelm_blocksize */
	elmp->orig_block = DLOOP_Stackelm_blocksize(elmp);

	if (i < depth-1) {
	    assert (!(dlp->kind & DLOOP_FINAL_MASK));

	    dlp = dlp->loop_params.cm_t.dataloop;
	}
	else assert(dlp->kind & DLOOP_FINAL_MASK);
    }

    segp->valid_sp = depth-1;

    return 0;
}

/* Segment_alloc
 *
 */
struct DLOOP_Segment * PREPEND_PREFIX(Segment_alloc)(void)
{
    return (struct DLOOP_Segment *) DLOOP_Malloc(sizeof(struct DLOOP_Segment));
}

/* Segment_free
 *
 * Input Parameters:
 * segp - pointer to segment
 */
void PREPEND_PREFIX(Segment_free)(struct DLOOP_Segment *segp)
{
    DLOOP_Free(segp);
    return;
}

/* DLOOP_Segment_manipulate - do something to a segment
 *
 * If you think of all the data to be manipulated (packed, unpacked, whatever),
 * as a stream of bytes, it's easier to understand how first and last fit in.
 *
 * This function does all the work, calling the piecefn passed in when it 
 * encounters a datatype element which falls into the range of first..(last-1).
 *
 * piecefn can be NULL, in which case this function doesn't do anything when it
 * hits a region.  This is used internally for repositioning within this stream.
 *
 * last is a byte offset to the byte just past the last byte in the stream 
 * to operate on.  this makes the calculations all over MUCH cleaner.
 *
 * stream_off, stream_el_size, first, and last are all working in terms of the
 * types and sizes for the stream, which might be different from the local sizes
 * (in the heterogeneous case).
 *
 * This is a horribly long function.  Too bad; it's complicated :)! -- Rob
 *
 * NOTE: THIS IMPLEMENTATION CANNOT HANDLE STRUCT DATALOOPS.
 */
#define DLOOP_SEGMENT_SAVE_LOCAL_VALUES		\
do {						\
    segp->cur_sp     = cur_sp;			\
    segp->valid_sp   = valid_sp;		\
    segp->stream_off = stream_off;		\
    *lastp           = stream_off;		\
} while (0)

#define DLOOP_SEGMENT_LOAD_LOCAL_VALUES		\
do {						\
    last       = *lastp;			\
    cur_sp     = segp->cur_sp;			\
    valid_sp   = segp->valid_sp;		\
    stream_off = segp->stream_off;		\
    cur_elmp   = &(segp->stackelm[cur_sp]);	\
} while (0)

#define DLOOP_SEGMENT_RESET_VALUES						    \
do {										    \
    segp->stream_off     = 0;							    \
    segp->cur_sp         = 0; 							    \
    cur_elmp             = &(segp->stackelm[0]);				    \
    cur_elmp->curcount   = cur_elmp->orig_count;				    \
    cur_elmp->orig_block = DLOOP_Stackelm_blocksize(cur_elmp);			    \
    cur_elmp->curblock   = cur_elmp->orig_block;				    \
    cur_elmp->curoffset  = cur_elmp->orig_offset + DLOOP_Stackelm_offset(cur_elmp); \
} while (0)

#define DLOOP_SEGMENT_POP_AND_MAYBE_EXIT			\
do {								\
    cur_sp--;							\
    if (cur_sp >= 0) cur_elmp = &segp->stackelm[cur_sp];	\
    else {							\
	DLOOP_SEGMENT_SAVE_LOCAL_VALUES;			\
	return;							\
    }								\
} while (0)

#define DLOOP_SEGMENT_PUSH			\
do {						\
    cur_sp++;					\
    cur_elmp = &segp->stackelm[cur_sp];		\
} while (0)

#define DLOOP_STACKELM_BLOCKINDEXED_OFFSET(__elmp, __curcount) \
(__elmp)->loop_p->loop_params.bi_t.offset_array[(__curcount)]

#define DLOOP_STACKELM_INDEXED_OFFSET(__elmp, __curcount) \
(__elmp)->loop_p->loop_params.i_t.offset_array[(__curcount)]

#define DLOOP_STACKELM_INDEXED_BLOCKSIZE(__elmp, __curcount) \
(__elmp)->loop_p->loop_params.i_t.blocksize_array[(__curcount)]

void PREPEND_PREFIX(Segment_manipulate)(struct DLOOP_Segment *segp,
					DLOOP_Offset first, 
					DLOOP_Offset *lastp, 
					int (*contigfn) (DLOOP_Offset *blocks_p,
							 DLOOP_Type el_type,
							 DLOOP_Offset rel_off,
							 void *bufp,
							 void *v_paramp),
					int (*vectorfn) (DLOOP_Offset *blocks_p,
							 int count,
							 int blklen,
							 DLOOP_Offset stride,
							 DLOOP_Type el_type,
							 DLOOP_Offset rel_off,
							 void *bufp,
							 void *v_paramp),
					int (*indexfn) (DLOOP_Offset *blocks_p,
							int count,
							int *blockarray,
							DLOOP_Offset *offsetarray,
							DLOOP_Type el_type,
							DLOOP_Offset rel_off,
							void *bufp,
							void *v_paramp),
					DLOOP_Offset (*sizefn) (DLOOP_Type el_type),
					void *pieceparams)
{
    /* these four are the "local values": cur_sp, valid_sp, last, stream_off */
    int cur_sp, valid_sp;
    DLOOP_Offset last, stream_off;

    struct DLOOP_Dataloop_stackelm *cur_elmp;
    enum { PF_NULL, PF_CONTIG, PF_VECTOR, PF_BLOCKINDEXED, PF_INDEXED } piecefn_type = PF_NULL;

    DLOOP_SEGMENT_LOAD_LOCAL_VALUES;

    if (first == *lastp) {
	/* nothing to do */
	DLOOP_dbg_printf("dloop_segment_manipulate: warning: first == last (%d)\n", (int) first);
	return;
    }

    /* first we ensure that stream_off and first are in the same spot */
    if (first != stream_off) {
#ifdef DEBUG_DLOOP_MANIPULATE
	DLOOP_dbg_printf("first=%d; stream_off=%ld; resetting.\n", first, stream_off);
#endif

	if (first < stream_off) {
	    DLOOP_SEGMENT_RESET_VALUES;
	    stream_off = 0;
	}

	if (first != stream_off) {
	    DLOOP_Offset tmp_last = first;

	    /* use manipulate function with a NULL piecefn to advance stream offset */
	    PREPEND_PREFIX(Segment_manipulate)(segp,
					       stream_off,
					       &tmp_last,
					       NULL,
					       NULL,
					       NULL,
					       sizefn,
                                               NULL);
	    
	    /* verify that we're in the right location */
	    if (tmp_last != first) assert(0);
	}

	DLOOP_SEGMENT_LOAD_LOCAL_VALUES;

#ifdef DEBUG_DLOOP_MANIPULATE
	DLOOP_dbg_printf("done repositioning stream_off; first=%d, stream_off=%ld, last=%d\n",
		   first, stream_off, last);
#endif
    }

    for (;;) {
#ifdef DEBUG_DLOOP_MANIPULATE
        DLOOP_dbg_printf("looptop; cur_sp=%d, cur_elmp=%x\n",
			 cur_sp, (unsigned) cur_elmp);
#endif

	if (cur_elmp->loop_p->kind & DLOOP_FINAL_MASK) {
	    int piecefn_indicated_exit = -1;
	    DLOOP_Offset myblocks, local_el_size, stream_el_size;
	    DLOOP_Type el_type;

	    /* pop immediately on zero count */
	    if (cur_elmp->curcount == 0) DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;

	    /* size on this system of the int, double, etc. that is the elem. type */
	    local_el_size  = cur_elmp->loop_p->el_size;
	    el_type        = cur_elmp->loop_p->el_type;
	    stream_el_size = (sizefn) ? sizefn(el_type) : local_el_size;

	    /* calculate number of elem. types to work on and function to use.
	     * default is to use the contig piecefn (if there is one).
	     */
	    myblocks = cur_elmp->curblock;
	    piecefn_type = (contigfn ? PF_CONTIG : PF_NULL);

	    /* check for opportunities to use other piecefns */
	    switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		    break;
         	case DLOOP_KIND_BLOCKINDEXED:
		    break;
		case DLOOP_KIND_INDEXED:
		    /* only use index piecefn if at start of the index type */
		    if (indexfn &&
			cur_elmp->orig_block == cur_elmp->curblock &&
			cur_elmp->orig_count == cur_elmp->curcount)
		    {
			/* TODO: RELAX CONSTRAINT ON COUNT? */
			myblocks = cur_elmp->loop_p->loop_params.i_t.total_blocks;
			piecefn_type = PF_INDEXED;
		    }
		    break;
		case DLOOP_KIND_VECTOR:
		    /* only use the vector piecefn if at the start of a
		     * contiguous block.
		     */
		    if (vectorfn && cur_elmp->orig_block == cur_elmp->curblock) {
			myblocks = cur_elmp->curblock * cur_elmp->curcount;
			piecefn_type = PF_VECTOR;
		    }
		    break;
		default:
		    assert(0);
	    }

#ifdef DEBUG_DLOOP_MANIPULATE
	    DLOOP_dbg_printf("\thit leaf; cur_sp=%d, elmp=%x, piece_sz=%d\n", cur_sp,
		       (unsigned) cur_elmp, myblocks * local_el_size);
#endif

	    /* enforce the last parameter if necessary by reducing myblocks */
	    if (last != SEGMENT_IGNORE_LAST &&
		(stream_off + (myblocks * stream_el_size) > last))
	    {
		myblocks = ((last - stream_off) / stream_el_size);
#ifdef DEBUG_DLOOP_MANIPULATE
		DLOOP_dbg_printf("\tpartial block count=%d\n", myblocks);
#endif
		if (myblocks == 0) {
		    DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		    return;
		}
	    }

	    /* call piecefn to perform data manipulation */
	    switch (piecefn_type) {
		case PF_NULL:
		    piecefn_indicated_exit = 0;
#ifdef DEBUG_DLOOP_MANIPULATE
		    DLOOP_dbg_printf("\tNULL piecefn for this piece\n");
#endif
		    break;
		case PF_CONTIG:
		    assert(myblocks <= cur_elmp->curblock);
		    piecefn_indicated_exit =
			contigfn(&myblocks,
				 el_type,
				 cur_elmp->curoffset, /* relative to segp->ptr */
				 segp->ptr, /* start of buffer (from segment) */
				 pieceparams);
		    break;
		case PF_VECTOR:
		    piecefn_indicated_exit =
			vectorfn(&myblocks,
				 cur_elmp->curcount,
				 cur_elmp->orig_block,
				 cur_elmp->loop_p->loop_params.v_t.stride,
				 el_type,
				 cur_elmp->curoffset, /* relative to segp->ptr */
				 segp->ptr, /* start of buffer (from segment) */
				 pieceparams);
		    break;
		case PF_BLOCKINDEXED:
		    assert(0);
		    break;
		case PF_INDEXED:
		    piecefn_indicated_exit =
			indexfn(&myblocks,
				cur_elmp->curcount,
				cur_elmp->loop_p->loop_params.i_t.blocksize_array,
				cur_elmp->loop_p->loop_params.i_t.offset_array,
				el_type,
				cur_elmp->orig_offset, /* indexfn adds offset value */
				segp->ptr,
				pieceparams);
		    break;
	    }

	    /* update local values based on piecefn returns (myblocks and
	     * piecefn_indicated_exit)
	     */
	    assert(piecefn_indicated_exit >= 0);
	    assert(myblocks >= 0);
	    stream_off += myblocks * stream_el_size;

	    if (myblocks == 0) {
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }
	    else if (myblocks < cur_elmp->curblock) {
		cur_elmp->curoffset += myblocks * local_el_size;
		cur_elmp->curblock  -= myblocks;

		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }
	    else /* myblocks >= cur_elmp->curblock */ {
		int count_index = 0;

		/* this assumes we're either *just* processing the last parts
		 * of the current block, or we're processing as many blocks as
		 * we like starting at the beginning of one.
		 */
		assert(myblocks == cur_elmp->curblock ||
		       cur_elmp->curblock == cur_elmp->orig_block);

		switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		    case DLOOP_KIND_INDEXED:
			while (myblocks > 0 && myblocks >= cur_elmp->curblock) {
			    myblocks -= cur_elmp->curblock;
			    cur_elmp->curcount--;
			    assert(cur_elmp->curcount >= 0);

			    count_index = cur_elmp->orig_count - cur_elmp->curcount;
			    cur_elmp->curblock =
				DLOOP_STACKELM_INDEXED_BLOCKSIZE(cur_elmp, count_index);
			}

			if (cur_elmp->curcount == 0) {
			    /* don't bother to fill in values; we're popping anyway */
			    assert(myblocks == 0);
			    DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
			}
			else {
			    cur_elmp->orig_block = cur_elmp->curblock;
			    cur_elmp->curoffset  = cur_elmp->orig_offset +
				DLOOP_STACKELM_INDEXED_OFFSET(cur_elmp, count_index);
			    
			    cur_elmp->curblock  -= myblocks;
			    cur_elmp->curoffset += myblocks * local_el_size;
			}
			break;
		    case DLOOP_KIND_VECTOR:
			/* this math relies on assertions at top of code block */
			cur_elmp->curcount -= myblocks / cur_elmp->curblock;
			if (cur_elmp->curcount == 0) {
			    assert(myblocks % cur_elmp->orig_block == 0);
			    DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
			}
			else {
			    /* this math relies on assertions at top of code block */
			    cur_elmp->curblock = cur_elmp->orig_block - (myblocks % cur_elmp->curblock);
			    /* new offset = original offset +
			     *              stride * whole blocks +
			     *              leftover bytes
			     */
			    cur_elmp->curoffset = cur_elmp->orig_offset +
				((cur_elmp->orig_count - cur_elmp->curcount) *
				 cur_elmp->loop_p->loop_params.v_t.stride) +
				((cur_elmp->orig_block - cur_elmp->curblock) *
				 local_el_size);
			}
			break;
		    case DLOOP_KIND_CONTIG:
			/* contigs that reach this point have always been completely processed */
			assert(myblocks == cur_elmp->curblock &&
			       cur_elmp->curcount == 1);
			DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
			break;
		    case DLOOP_KIND_BLOCKINDEXED:
			while (myblocks > 0 && myblocks >= cur_elmp->curblock) {
			    myblocks -= cur_elmp->curblock;
			    cur_elmp->curcount--;
			    assert(cur_elmp->curcount >= 0);

			    count_index = cur_elmp->orig_count - cur_elmp->curcount;
			    cur_elmp->curblock = cur_elmp->orig_block;
			}
			if (cur_elmp->curcount == 0) {
			    /* popping */
			    assert(myblocks == 0);
			    DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
			}
			else {
			    /* cur_elmp->orig_block = cur_elmp->curblock; */
			    cur_elmp->curoffset = cur_elmp->orig_offset +
				DLOOP_STACKELM_BLOCKINDEXED_OFFSET(cur_elmp, count_index);
			    cur_elmp->curblock  -= myblocks;
			    cur_elmp->curoffset += myblocks * local_el_size;
			}
			break;
		}
	    }

	    if (piecefn_indicated_exit) {
		/* The piece function indicated that we should quit processing */
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }
	} /* end of if leaf */
	else if (cur_elmp->curblock == 0) {
#ifdef DEBUG_DLOOP_MANIPULATE
	    DLOOP_dbg_printf("\thit end of block; elmp=%x [%d]\n",
			    (unsigned) cur_elmp, cur_sp);
#endif
	    cur_elmp->curcount--;
	    if (cur_elmp->curcount == 0) {
#ifdef DEBUG_DLOOP_MANIPULATE
		DLOOP_dbg_printf("\talso hit end of count; elmp=%x [%d]\n",
				(unsigned) cur_elmp, cur_sp);
#endif
		DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
	    }
	    else {
		/* new block.  for indexed and struct reset orig_block.  reset curblock for all types */
		switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		    case DLOOP_KIND_INDEXED:
			cur_elmp->orig_block =
			    DLOOP_STACKELM_INDEXED_BLOCKSIZE(cur_elmp, cur_elmp->orig_count - cur_elmp->curcount);
			break;
		    case DLOOP_KIND_VECTOR:
		    case DLOOP_KIND_BLOCKINDEXED:
		    case DLOOP_KIND_CONTIG:
			break;
		    case DLOOP_KIND_STRUCT:
		    default:
			assert(0);
			break;
		}
		cur_elmp->curblock = cur_elmp->orig_block;
	    }
	}
	else /* push the stackelm */ {
	    DLOOP_Dataloop_stackelm *next_elmp;
	    int count_index, block_index;

	    /* assuming that stack has been preloaded here. */
	    next_elmp   = &(segp->stackelm[cur_sp + 1]);

	    count_index = cur_elmp->orig_count - cur_elmp->curcount;
	    block_index = cur_elmp->orig_block - cur_elmp->curblock;

#ifdef DEBUG_DLOOP_MANIPULATE
	    DLOOP_dbg_printf("\tpushing type, elmp=%x [%d], count=%d, block=%d\n",
			    (unsigned) cur_elmp, cur_sp, count_index, block_index);
#endif
	    /* set orig_offset and all cur values for new stackelm.
	     * this is done in two steps: first set orig_offset based on
	     * current stackelm, then set cur values based on new stackelm.
	     */
	    switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		    next_elmp->orig_offset = cur_elmp->curoffset +
			block_index * cur_elmp->loop_p->el_extent;
		    break;
		case DLOOP_KIND_VECTOR:
		    /* note: stride is in bytes */
		    next_elmp->orig_offset = cur_elmp->orig_offset +
			count_index * cur_elmp->loop_p->loop_params.v_t.stride +
			block_index * cur_elmp->loop_p->el_extent;
		    break;
		case DLOOP_KIND_BLOCKINDEXED:
		    next_elmp->orig_offset = cur_elmp->orig_offset +
			block_index * cur_elmp->loop_p->el_extent +
			DLOOP_STACKELM_BLOCKINDEXED_OFFSET(cur_elmp, count_index);
		    break;
		case DLOOP_KIND_INDEXED:
		    next_elmp->orig_offset = cur_elmp->orig_offset +
			block_index * cur_elmp->loop_p->el_extent +
			DLOOP_STACKELM_INDEXED_OFFSET(cur_elmp, count_index);
		    break;
	    }

#ifdef DEBUG_DLOOP_MANIPULATE
	    DLOOP_dbg_printf("\tstep 1: next orig_offset = %d (0x%x)\n",
			     next_elmp->orig_offset,
			     next_elmp->orig_offset);
#endif

	    switch (next_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		case DLOOP_KIND_VECTOR:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curblock  = next_elmp->orig_block;
		    next_elmp->curoffset = next_elmp->orig_offset;
		    break;
		case DLOOP_KIND_BLOCKINDEXED:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curblock  = next_elmp->orig_block;
		    next_elmp->curoffset = next_elmp->orig_offset +
			DLOOP_STACKELM_BLOCKINDEXED_OFFSET(next_elmp, 0);
		    break;
		case DLOOP_KIND_INDEXED:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curblock  = DLOOP_STACKELM_INDEXED_BLOCKSIZE(next_elmp, 0);
		    next_elmp->curoffset = next_elmp->orig_offset + DLOOP_STACKELM_INDEXED_OFFSET(next_elmp, 0);
		    break;
	    }

#ifdef DEBUG_DLOOP_MANIPULATE
	    DLOOP_dbg_printf("\tstep 2: next curoffset = %d (0x%x)\n",
			     next_elmp->curoffset,
			     next_elmp->curoffset);
#endif

	    cur_elmp->curblock--;
	    DLOOP_SEGMENT_PUSH;
	} /* end of else push the stackelm */
    } /* end of for (;;) */

#ifdef DEBUG_DLOOP_MANIPULATE
    DLOOP_dbg_printf("hit end of datatype\n");
#endif

    DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
    return;
}

/* DLOOP_Stackelm_blocksize - returns block size for stackelm based on current
 * count in stackelm.
 *
 * NOTE: loop_p, orig_count, and curcount members of stackelm MUST be correct
 * before this is called!
 *
 */
static inline int DLOOP_Stackelm_blocksize(struct DLOOP_Dataloop_stackelm *elmp)
{
    struct DLOOP_Dataloop *dlp = elmp->loop_p;
       
    switch(dlp->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_CONTIG:
	    return dlp->loop_params.c_t.count; /* NOTE: we're dropping the count into the
						* blksize field for contigs, as described
						* in the init call.
						*/
	    break;
	case DLOOP_KIND_VECTOR:
	    return dlp->loop_params.v_t.blocksize;
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    return dlp->loop_params.bi_t.blocksize;
	    break;
	case DLOOP_KIND_INDEXED:
	    return dlp->loop_params.i_t.blocksize_array[elmp->orig_count - elmp->curcount];
	    break;
	case DLOOP_KIND_STRUCT:
	    return dlp->loop_params.s_t.blocksize_array[elmp->orig_count - elmp->curcount];
	    break;
	default:
	    assert(0);
	    break;
    }
    return -1;
}

/* DLOOP_Stackelm_offset - returns starting offset (displacement) for stackelm
 * based on current count in stackelm.
 *
 * NOTE: loop_p, orig_count, and curcount members of stackelm MUST be correct
 * before this is called!
 *
 * also, this really is only good at init time for vectors and contigs 
 * (all the time for indexed) at the moment.
 *
 */
static inline int DLOOP_Stackelm_offset(struct DLOOP_Dataloop_stackelm *elmp)
{
    struct DLOOP_Dataloop *dlp = elmp->loop_p;
       
    switch(dlp->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_VECTOR:
	case DLOOP_KIND_CONTIG:
	    return 0;
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    return dlp->loop_params.bi_t.offset_array[elmp->orig_count - elmp->curcount];
	    break;
	case DLOOP_KIND_INDEXED:
	    return dlp->loop_params.i_t.offset_array[elmp->orig_count - elmp->curcount];
	    break;
	case DLOOP_KIND_STRUCT:
	    return dlp->loop_params.s_t.offset_array[elmp->orig_count - elmp->curcount];
	    break;
	default:
	    assert(0);
	    break;
    }
    return -1;
}

/* 
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */
