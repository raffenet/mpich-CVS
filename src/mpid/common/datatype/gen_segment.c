/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* NOTE: This is kind-of a hack; it would be better to somehow get this
 * included on the compile line.
 */
#include <mpid_dataloop.h>
#include <mpiimpl.h>

#undef DLOOP_M_VERBOSE

#ifndef GEN_DATALOOP_H
#error "You must explicitly include a header that sets the PREPEND_PREFIX and includes gen_dataloop.h"
#endif

/* Notes on functions:
 *
 * There are a few different sets of functions here:
 * - Internal "piece" functions - helper functions for performing different 
 *   operations via segments
 * - DLOOP_Segment_manipulate() - uses a "piece" function to perform operations
 *   using segments
 * - PREPEND_PREFIX functions - these define the externally visible interface
 *   to segment functionality
 */

static inline int DLOOP_Stackelm_blocksize(struct DLOOP_Dataloop_stackelm *elmp);
static inline int DLOOP_Stackelm_offset(struct DLOOP_Dataloop_stackelm *elmp);

/*
 * NOT USING OPTIMIZED DATALOOPS YET (SINCE THEY DON'T EXIST <SMILE>)
 */

/* Segment_init
 *
 * buf    - datatype buffer location
 * count  - number of instances of the datatype in the buffer
 * handle - handle for datatype (could be derived or not)
 * segp   - pointer to previously allocated segment structure
 *
 * Assumes that the segment has been allocated.
 *
 */
int PREPEND_PREFIX(Segment_init)(const DLOOP_Buffer buf,
				 DLOOP_Count count,
				 DLOOP_Handle handle, 
				 struct DLOOP_Segment *segp)
{
    int i, elmsize, depth = 0;
    struct DLOOP_Dataloop_stackelm *elmp;
    struct DLOOP_Dataloop *dlp = 0;
    
    /* first figure out what to do with the datatype/count.
     * there are three cases:
     * - predefined datatype, any count; use the builtin loop only
     * - derived type, count == 1; don't use builtin at all
     * - derived type, count > 1; use builtin for contig of derived type
     */

    if (!DLOOP_Handle_hasloop_macro(handle)) {
	/* simplest case; datatype has no loop (basic) */

	DLOOP_Handle_get_size_macro(handle, elmsize);

	/* NOTE: ELMSIZE IS WRONG */
	segp->builtin_loop.kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK 
	    | (elmsize << DLOOP_ELMSIZE_SHIFT);
	segp->builtin_loop.handle = handle;
	segp->builtin_loop.loop_params.c_t.count = count;
	segp->builtin_loop.loop_params.c_t.u.dataloop = 0;
	segp->builtin_loop.el_size = elmsize;
	DLOOP_Handle_get_extent_macro(handle, segp->builtin_loop.el_extent);

	dlp = &segp->builtin_loop;
	depth = 1;
    }
    else if (count == 1) {
	/* don't use the builtin */
	DLOOP_Handle_get_loopptr_macro(handle, dlp);
	DLOOP_Handle_get_loopdepth_macro(handle, depth);
    }
    else {
	/* need to use builtin to handle contig; must check loop depth first */
	
	DLOOP_Handle_get_loopdepth_macro(handle, depth);
	if (depth >= DLOOP_MAX_DATATYPE_DEPTH) return -1;

	depth++; /* we're adding to the depth with the builtin */

	DLOOP_Handle_get_size_macro(handle, elmsize);
	/* NOTE: ELMSIZE IS WRONG */
	segp->builtin_loop.kind = DLOOP_KIND_CONTIG 
	    | (elmsize << DLOOP_ELMSIZE_SHIFT);
	segp->builtin_loop.loop_params.c_t.count = count;
	DLOOP_Handle_get_loopptr_macro(handle, segp->builtin_loop.loop_params.c_t.u.dataloop);
	segp->builtin_loop.el_size = elmsize;
	DLOOP_Handle_get_extent_macro(handle, segp->builtin_loop.el_extent);

	dlp = &segp->builtin_loop;
    }

    /* initialize the rest of the segment values */
    segp->handle = handle;
    segp->ptr = (DLOOP_Buffer) buf;
    segp->stream_off = 0;
    segp->cur_sp = 0;
    segp->valid_sp = 0;

    for (i=0; i < depth; i++) {
	elmp = &(segp->stackelm[i]);
	/* fill in stackelms.  this won't be quite right for structs.
	 *
	 * We always initialize our curcount to the count in the loop_p
	 * because this makes testing for completion on each iteration against
	 * 0 instead of against the value stored off in the loop.
	 *
	 * Same argument applies to curblock.  DLOOP_Stackelm_blocksize(elmp) takes
	 * care of this; the loop_p field MUST be filled in for it to work.
	 *
	 * This does make things a little more difficult when count is used as
	 * an index into something, but I think the savings in the other cases 
	 * outweigh this.
	 *
	 * NOTE: a bunch of these values aren't correct; we're getting some values
	 * into place so that they don't have to be filled in later, and others for
	 * the topmost dataloop...the rest are junk.
	 */
	elmp->curcount    = dlp->loop_params.count;
	elmp->orig_count  = elmp->curcount;
	elmp->loop_p      = dlp; /* DO NOT MOVE THIS BELOW THE Stackelm CALLS! */

	elmp->orig_offset = 0;
	elmp->curoffset   = /* elmp->orig_offset + */ DLOOP_Stackelm_offset(elmp);
	elmp->curblock    = DLOOP_Stackelm_blocksize(elmp);
	elmp->orig_block  = elmp->curblock;

	if (i < depth-1) {
	    /* not at last point in the stack */
	    if (dlp->kind & DLOOP_FINAL_MASK) assert(0);

	    if ((dlp->kind & DLOOP_KIND_MASK) == DLOOP_KIND_STRUCT) {
		dlp = dlp->loop_params.s_t.dataloop_array[0];
	    }
	    else {
		dlp = dlp->loop_params.cm_t.u.dataloop;
	    }
	}
	else {
	    /* last in stack */
	    if (!(dlp->kind & DLOOP_FINAL_MASK)) assert(0);

	    if ((dlp->kind & DLOOP_KIND_MASK) == DLOOP_KIND_STRUCT) {
		assert(0);
	    }
	    else {
		dlp = NULL; /* ??? do we need the last handle somehow? 
			     * or is the el_size and el_extent enough?
			     */
	    }
	}
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
 * This function does all the work, calling the piecefn passed in when it 
 * encounters a datatype element which falls into the range of first..(last-1).
 *
 * piecefn can be NULL, in which case this function doesn't do anything when it
 * hits a region.
 *
 * last is a byte offset to the byte just past the last byte in the stream 
 * to operate on.  this makes the calculations all over MUCH cleaner.
 *
 * this is a horribly long function.  sorry!
 */
#define DLOOP_SEGMENT_SAVE_LOCAL_VALUES \
do { \
    segp->cur_sp = cur_sp; \
    segp->valid_sp = valid_sp; \
    segp->stream_off = stream_off; \
    *lastp = stream_off; \
} while (0)

#define DLOOP_SEGMENT_LOAD_LOCAL_VALUES \
do { \
    last = *lastp; \
    cur_sp = segp->cur_sp; \
    valid_sp = segp->valid_sp; \
    stream_off = segp->stream_off; \
} while (0)

/* NOTE: NO GOOD FOR STRUCTS!!!
 *
 * ALSO, DOESN'T HANDLE LB YET.
 */
#define DLOOP_STACKELM_RESET_VALUES \
do { \
    cur_elmp->curcount   = cur_elmp->orig_count; \
    cur_elmp->orig_block = DLOOP_Stackelm_blocksize(cur_elmp); \
    cur_elmp->curblock   = cur_elmp->orig_block; \
    cur_elmp->curoffset  = cur_elmp->orig_offset + DLOOP_Stackelm_offset(cur_elmp); \
} while (0)

void PREPEND_PREFIX(Segment_manipulate)(struct DLOOP_Segment *segp,
					int first, 
					int *lastp, 
					int (*piecefn)(DLOOP_Handle,
						       int,
						       int,
						       void *,
						       void *), 
					void *pieceparams)
{
    int ret, piece_size, basic_size, dtype_size, partial_flag;
    struct DLOOP_Dataloop_stackelm *cur_elmp, *next_elmp;

    /* segment local values */
    int last, cur_sp, valid_sp;
    unsigned long stream_off;

    DLOOP_SEGMENT_LOAD_LOCAL_VALUES;

    /* first we ensure that stream_off and first are in the same spot */
    if (first != (int)stream_off) {
	int tmp_last;

#ifdef DLOOP_M_VERBOSE
	MPIU_dbg_printf("first=%d; stream_off=%ld; resetting.\n", first, stream_off);
#endif

	/* TODO: BE SMARTER AND DON'T RESET IF STREAM_OFF IS BEFORE FIRST */
	/* reset to beginning of stream */
	segp->stream_off = 0;
	segp->cur_sp = 0;
	cur_elmp = &(segp->stackelm[0]);
	DLOOP_STACKELM_RESET_VALUES;

	/* TODO: AVOID CALLING MANIP. IF FIRST == 0 */

	/* Note: we're simply using the manipulate function with a NULL piecefn
	 * to handle this case.  It's one more function call, but it dramatically
	 * simplifies this code.
	 */
	tmp_last = first;
	PREPEND_PREFIX(Segment_manipulate)(segp, 0, &tmp_last, NULL, NULL);
	
	/* verify that we're in the right location */
	if (tmp_last != first) assert(0);

	DLOOP_SEGMENT_LOAD_LOCAL_VALUES;

	/* continue processing... */
#ifdef DLOOP_M_VERBOSE
	MPIU_dbg_printf("done repositioning stream_off; first=%d, stream_off=%ld, last=%d\n",
		   first, stream_off, last);
#endif
    }

    while (cur_sp >= 0) {
	cur_elmp = &(segp->stackelm[cur_sp]);

	/* no structs in here */
	if ((cur_elmp->loop_p->kind & DLOOP_KIND_MASK) == DLOOP_KIND_STRUCT) assert(0);

#ifdef DLOOP_M_VERBOSE
        MPIU_dbg_printf("looptop; cur_sp=%d, cur_elmp=%x\n", cur_sp, (unsigned) cur_elmp);
#endif

	if (cur_elmp->loop_p->kind & DLOOP_FINAL_MASK) {
	    /* process data region */

	    /* First discover how large a region we *could* process, if it
	     * could all be handled by the processing function.
	     */
	    dtype_size = cur_elmp->loop_p->el_size;

	    /* this is the fundamental size at which we should work.
	     * this could theoretically be smaller than the 
	     * dtype size; dunno yet.  if so, it will be a big mess
	     * to keep up with...
	     *
	     * TODO: GET THIS FROM THE TYPE MORE CORRECTLY
	     */
	    basic_size = cur_elmp->loop_p->el_size;

	    switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		    piece_size = cur_elmp->curcount * dtype_size;
		    break;
         	case DLOOP_KIND_BLOCKINDEXED:
		case DLOOP_KIND_INDEXED:
		    piece_size = cur_elmp->curblock * dtype_size;
		    break;
		case DLOOP_KIND_VECTOR:
		    /* TODO: RECOGNIZE ABILITY TO DO STRIDED COPIES --
		     * ONLY GOOD FOR THE NON-VECTOR CASES...
		     */
		    piece_size = cur_elmp->curblock * dtype_size;
		    break;
		default:
		    assert(0);
	    }

#ifdef DLOOP_M_VERBOSE
	    MPIU_dbg_printf("\thit leaf; cur_sp=%d, elmp=%x, piece_sz=%d\n", cur_sp,
		       (unsigned) cur_elmp, piece_size);
#endif

	    /* ??? SHOULD THIS BE >= FOR SOME REASON ??? */
	    if (stream_off + piece_size > (unsigned long) last) {
		/* Cannot process the entire "piece" -- round down */
		piece_size = ((last - stream_off) / basic_size) * basic_size;
#ifdef DLOOP_M_VERBOSE
		MPIU_dbg_printf("\tpartial piece_size=%d\n", piece_size);
#endif
		partial_flag = 1;
	    }
	    else {
		partial_flag = 0; /* handling everything left for this type */
	    }
	    if (piecefn) ret = piecefn((DLOOP_Handle) 0,
				       cur_elmp->curoffset,
				       piece_size,
				       segp->ptr,
				       pieceparams);
	    else {
		ret = 0;
#ifdef DLOOP_M_VERBOSE
		MPIU_dbg_printf("\tNULL piecefn for this piece\n");
#endif
	    }
	    stream_off += piece_size;

	    /* TODO: MAYBE REORGANIZE? */
	    if (partial_flag) {
		cur_elmp->curoffset += piece_size;

		/* definitely didn't process everything in this contig. region */
		/* NOTE: THIS CODE ASSUMES THAT WE'RE WORKING IN WHOLE DTYPE SIZES!!! */
		switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		    case DLOOP_KIND_CONTIG:
			cur_elmp->curcount -= piece_size / dtype_size;
			break;
		    case DLOOP_KIND_BLOCKINDEXED:
		    case DLOOP_KIND_INDEXED:
			cur_elmp->curblock -= piece_size / dtype_size;
			break;
		    case DLOOP_KIND_VECTOR:
			/* TODO: RECOGNIZE ABILITY TO DO STRIDED COPIES --
			 * ONLY GOOD FOR THE NON-VECTOR CASES...
			 */
			cur_elmp->curblock -= piece_size / dtype_size;
			break;
		    default:
			assert(0);
		}
#ifdef DLOOP_M_VERBOSE
		MPIU_dbg_printf("partial flag, returning sooner than expected.\n");
#endif
		ret = 1; /* forces return below */
	    }
	    else {
		/* we at least finished a whole block */
		/* Update the stack elements.  Either we're done with the count,
		 * in which case it is time to pop off, or we need to reset the
		 * block value (because we just handled an entire block).
		 *
		 * Note that this will get more complicated as I add the ability
		 * to handle more of the partial processing cases. ???
		 */
		switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		    case DLOOP_KIND_CONTIG:
			cur_elmp->curoffset += piece_size;
			cur_sp--; /* currently always handling the whole contig */
			break;
		    case DLOOP_KIND_BLOCKINDEXED:
			assert(0);
			break;
		    case DLOOP_KIND_INDEXED:
			cur_elmp->curcount--;
			if (cur_elmp->curcount == 0) cur_sp--;
			else {
			    cur_elmp->orig_block = DLOOP_Stackelm_blocksize(cur_elmp);
			    cur_elmp->curblock   = cur_elmp->orig_block;
			    cur_elmp->curoffset  = cur_elmp->orig_offset +
				DLOOP_Stackelm_offset(cur_elmp);
#ifdef DLOOP_M_VERBOSE
			    MPIU_dbg_printf("\tnew region: origoff = %d; curoff = %d; blksz = %d\n",
					    (int) cur_elmp->orig_offset,
					    (int) cur_elmp->curoffset,
					    (int) cur_elmp->curblock);
#endif
			}
			break;
		    case DLOOP_KIND_VECTOR:
			cur_elmp->curcount--;
			if (cur_elmp->curcount == 0) cur_sp--;
			else {
#ifdef DLOOP_M_VERBOSE
			    MPIU_dbg_printf("end of vec block; incrementing curoffset by %d\n",
					    cur_elmp->loop_p->loop_params.v_t.stride -
					    (cur_elmp->orig_block * cur_elmp->loop_p->el_size));
#endif

			    cur_elmp->curblock = cur_elmp->orig_block;
			    /* NOTE: stride is in bytes */
			    /* TODO: CLEAN THIS ONE UP */
			    cur_elmp->curoffset += piece_size;
			    cur_elmp->curoffset += cur_elmp->loop_p->loop_params.v_t.stride -
				(cur_elmp->orig_block * cur_elmp->loop_p->el_size);
			}
			break;
		    default:
			assert(0);
		}
	    }
		
	    if (ret) {
		/* The piece function indicated that we should quit processing */
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }
	    /* NOTE: MUST HIT TOP OF LOOP NEXT */

	} /* end of if leaf */
	else if (cur_elmp->curblock == 0) {
	    /* This section is testing to see if we hit the end of the count
	     * for this type (which is not a leaf).  The first test is that we
	     * hit end of block.
	     */
#ifdef DLOOP_M_VERBOSE
	    MPIU_dbg_printf("\thit end of block; elmp=%x\n",
			    (unsigned) cur_elmp);
#endif
	    cur_elmp->curcount--;
	    if (cur_elmp->curcount == 0) {
		/* We also hit end of count; pop this type. */
#ifdef DLOOP_M_VERBOSE
		MPIU_dbg_printf("\thit end of count; elmp=%x\n",
				(unsigned) cur_elmp);
#endif
		cur_sp--;
		/* NOTE: CRITICAL THAT WE HIT TOP OF LOOP FROM HERE! */
	    }
	    else {
		/* Otherwise we just have a new block.  Reset block value. */
		if ((cur_elmp->loop_p->kind & DLOOP_KIND_MASK) == DLOOP_KIND_INDEXED)
		{
		    /* indexed and struct are the only ones for which this can change
		     * during processing.  and this code doesn't do structs...
		     */
		    cur_elmp->orig_block = DLOOP_Stackelm_blocksize(cur_elmp);
		}
		cur_elmp->curblock = cur_elmp->orig_block;
		/* TODO: COMBINE INTO NEXT BIG ELSE; WE'RE PROBABLY MAKING AN EXTRA PASS THROUGH OUR LOOPS */
	    }
	} /* end of "hit end of a block, maybe hit end of loop (count)" */
	else {
	    int count_index, block_index;

	    /* Push the datatype.
	     *
	     * Recall that all the stack elements have been filled in at init 
	     * time.  However, the offset must be filled in at each iteration.
	     */
	    next_elmp   = &(segp->stackelm[cur_sp + 1]);
	    count_index = cur_elmp->orig_count - cur_elmp->curcount;
	    block_index = cur_elmp->orig_block - cur_elmp->curblock;

#ifdef DLOOP_M_VERBOSE
	    MPIU_dbg_printf("\tpushing type, elmp=%x, count=%d, block=%d\n",
			    (unsigned) cur_elmp, count_index, block_index);
#endif
	    /* This is a two-step process; some items are dependent on
	     * the current type, while others are dependent on the next
	     * type.
	     *
	     * First step: set up the orig_offset of next type based on
	     * the current type.
	     *
	     * If we weren't doing indexed, we could simpify this quite a bit.
	     */
	    switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		    next_elmp->orig_offset = cur_elmp->curoffset +
			count_index * cur_elmp->loop_p->el_extent;
#ifdef DLOOP_M_VERBOSE
		    MPIU_dbg_printf("outer contig el_size = %d, el_extent = %d\n",
				    (int) cur_elmp->loop_p->el_size,
				    (int) cur_elmp->loop_p->el_extent);
#endif
		    break;
		case DLOOP_KIND_VECTOR:
		    /* NOTE: stride is in bytes */
		    next_elmp->orig_offset = cur_elmp->curoffset +
			count_index * cur_elmp->loop_p->loop_params.v_t.stride +
			block_index * cur_elmp->loop_p->el_extent;
#ifdef DLOOP_M_VERBOSE
		    MPIU_dbg_printf("outer vec el_size = %d, el_extent = %d, stride = %d\n",
				    (int) cur_elmp->loop_p->el_size,
				    (int) cur_elmp->loop_p->el_extent,
				    (int) cur_elmp->loop_p->loop_params.v_t.stride);
#endif
		    break;
		case DLOOP_KIND_BLOCKINDEXED:
		    assert(0);
		    break;
		case DLOOP_KIND_INDEXED:
#if 0
		    next_elmp->orig_offset = cur_elmp->curoffset +
			DLOOP_Stackelm_offset(next_elmp);
#endif
		    /* Accounting for additional offset from being partway
		     * through a collection of blocks.
		     */
		    next_elmp->orig_offset = cur_elmp->curoffset + block_index * cur_elmp->loop_p->el_extent;
		    break;
		default:
		    assert(0);
	    } /* end of switch */

	    /* now we update the curoffset based on the next type */
	    switch (next_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curoffset = next_elmp->orig_offset;
		    next_elmp->curblock  = next_elmp->orig_block;
		    break;
		case DLOOP_KIND_VECTOR:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curoffset = next_elmp->orig_offset;
		    next_elmp->curblock  = next_elmp->orig_block;
		    break;
		case DLOOP_KIND_BLOCKINDEXED:
		    assert(0);
		    break;
		case DLOOP_KIND_INDEXED:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curoffset = next_elmp->orig_offset +
			DLOOP_Stackelm_offset(next_elmp);
		    next_elmp->curblock  = DLOOP_Stackelm_blocksize(next_elmp);
#ifdef DLOOP_M_VERBOSE
		    MPIU_dbg_printf("\treadying region: origoff = %d; curoff = %d; blksz = %d\n",
				    (int) next_elmp->orig_offset,
				    (int) next_elmp->curoffset,
				    (int) next_elmp->curblock);
#endif
		    break;
		default:
		    assert(0);
	    } /* end of switch */
	    /* TODO: HANDLE NON-ZERO OFFSETS IN NEXT_ELMP HERE? */


	    cur_elmp->curblock--;
	    cur_sp++; /* let cur_elmp be reset at top of loop */
	} /* end of else push the datatype */
    } /* end of while cur_sp >= 0 */

#ifdef DLOOP_M_VERBOSE
    MPIU_dbg_printf("hit end of datatype\n");
#endif

    DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
    return;
}

/* DLOOP_Stackelm_blocksize - returns block size for stackelm based on current
 * count in stackelm
 *
 * NOTE:
 * count and loop_p members of stackelm MUST be correct before this is 
 * called.
 *
 */
static inline int DLOOP_Stackelm_blocksize(struct DLOOP_Dataloop_stackelm *elmp)
{
    struct DLOOP_Dataloop *dlp = elmp->loop_p;
    int datatype_index;
       
    switch(dlp->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_VECTOR:
	    return dlp->loop_params.v_t.blocksize;
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    return dlp->loop_params.bi_t.blocksize;
	    break;
	case DLOOP_KIND_INDEXED:
	    datatype_index = elmp->loop_p->loop_params.count - elmp->curcount;
	    return dlp->loop_params.i_t.blocksize_array[datatype_index];
	    break;
	case DLOOP_KIND_STRUCT:
	    datatype_index = elmp->loop_p->loop_params.count - elmp->curcount;
	    return dlp->loop_params.s_t.blocksize_array[datatype_index];
	    break;
	case DLOOP_KIND_CONTIG:
	    return 1;
	    break;
	default:
	    assert(0);
	    break;
    }
    return -1;
}

/* DLOOP_Stackelm_offset - returns offset (displacement) for stackelm based
 * on current count in stackelm
 *
 * NOTE:
 * count and loop_p members of stackelm MUST be correct before this is 
 * called.
 *
 * also, this really is only good at init time for vectors and contigs 
 * (all the time for indexed) at the moment.
 *
 */
static inline int DLOOP_Stackelm_offset(struct DLOOP_Dataloop_stackelm *elmp)
{
    struct DLOOP_Dataloop *dlp = elmp->loop_p;
    int datatype_index;
       
    switch(dlp->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_VECTOR:
	case DLOOP_KIND_CONTIG:
	    return 0;
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    datatype_index = elmp->loop_p->loop_params.count - elmp->curcount;
	    return dlp->loop_params.bi_t.offset_array[datatype_index];
	    break;
	case DLOOP_KIND_INDEXED:
	    datatype_index = elmp->loop_p->loop_params.count - elmp->curcount;
	    return dlp->loop_params.i_t.offset_array[datatype_index];
	    break;
	case DLOOP_KIND_STRUCT:
	    datatype_index = elmp->loop_p->loop_params.count - elmp->curcount;
	    return dlp->loop_params.s_t.offset_array[datatype_index];
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

