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
    int i, elmsize, depth = 0;
    struct DLOOP_Dataloop_stackelm *elmp;
    struct DLOOP_Dataloop *dlp = 0;
    
    /* first figure out what to do with the datatype/count.
     * there are three cases:
     * - predefined datatype, any count; use the builtin loop only
     * - derived type, count == 1; don't use builtin at all
     * - derived type, count > 1; use builtin for contig of derived type
     */

#ifdef DLOOP_M_VERBOSE
    DLOOP_dbg_printf("DLOOP_Segment_init: count = %d, buf = %x\n",
		    count,
		    buf);
#endif

    if (!DLOOP_Handle_hasloop_macro(handle)) {
	/* simplest case; datatype has no loop (basic) */

	DLOOP_Handle_get_size_macro(handle, elmsize);

	/* NOTE: ELMSIZE IS WRONG */
	segp->builtin_loop.kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK | (elmsize << DLOOP_ELMSIZE_SHIFT);
	segp->builtin_loop.handle = handle;
	segp->builtin_loop.loop_params.c_t.count = count;
	segp->builtin_loop.loop_params.c_t.dataloop = 0;
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
	segp->builtin_loop.kind = DLOOP_KIND_CONTIG | (elmsize << DLOOP_ELMSIZE_SHIFT);
	segp->builtin_loop.loop_params.c_t.count = count;
	DLOOP_Handle_get_loopptr_macro(handle, segp->builtin_loop.loop_params.c_t.dataloop);
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

    /* initialize the first stackelm in its entirety */
    /* Note that we start counts (and blocks) at the maximum value and then decrement;
     * that way when we're testing for completion we don't have to look back at the
     * maximum value.
     */
    elmp = &(segp->stackelm[0]);
    elmp->loop_p = dlp;

    /* NOTE: It's easier later to use blksize for the size of a contig rather than count,
     * because in every other case blksize is the size of a contiguous set of types.
     */
    if ((dlp->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG) {
	elmp->orig_count = 1;
    }
    else {
	elmp->orig_count = dlp->loop_params.count;
    }
    
    elmp->curcount    = elmp->orig_count;
    elmp->orig_offset = 0;

    /* NOTE: orig_count, curcount, and loop_p MUST be correct in elm before calling
     * DLOOP_Stackelm_blocksize().
     */
    elmp->orig_block  = DLOOP_Stackelm_blocksize(elmp);
    elmp->curblock    = elmp->orig_block;
    /* NOTE: orig_count, curcount, and loop_p MUST be correct in elm before calling
     * DLOOP_Stackelm_offset().
     */
    elmp->curoffset   = /* elmp->orig_offset + */ DLOOP_Stackelm_offset(elmp);
    
    dlp = dlp->loop_params.cm_t.dataloop;

    for (i=1; i < depth; i++) {
	/* for the rest of the elements, we only fill in loop_p, orig_count, and
	 * orig_block.  the rest are filled in as we process the type (and for
	 * indexed case orig_block will be overwritten as well).
	 *
	 * except that we need to fill in curcount for the blocksize calculation
	 * to work correctly.
	 */
	elmp = &(segp->stackelm[i]);
	elmp->loop_p     = dlp; /* DO NOT MOVE THIS BELOW THE Stackelm CALLS! */
	if ((dlp->kind & DLOOP_KIND_MASK) == DLOOP_KIND_CONTIG) {
	    elmp->orig_count = 1;
	}
	else {
	    elmp->orig_count = dlp->loop_params.count;
	}
	elmp->curcount   = elmp->orig_count; /* NEEDED FOR BLOCKSIZE CALCULATION */
	elmp->orig_block = DLOOP_Stackelm_blocksize(elmp);

	if (i < depth-1) {
	    /* not at last point in the stack */
	    if (dlp->kind & DLOOP_FINAL_MASK) assert(0);

	    dlp = dlp->loop_params.cm_t.dataloop;
	}
	else if (!(dlp->kind & DLOOP_FINAL_MASK)) assert(0); /* sanity check */
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

#define DLOOP_SEGMENT_RESET_VALUES							\
do {											\
    segp->stream_off     = 0;								\
    segp->cur_sp         = 0; 								\
    cur_elmp             = &(segp->stackelm[0]);					\
    cur_elmp->curcount   = cur_elmp->orig_count;					\
    cur_elmp->orig_block = DLOOP_Stackelm_blocksize(cur_elmp);				\
    cur_elmp->curblock   = cur_elmp->orig_block;					\
    cur_elmp->curoffset  = cur_elmp->orig_offset + DLOOP_Stackelm_offset(cur_elmp);	\
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
					int (*contigfn) (int *blocks_p,
							 int el_size,
							 DLOOP_Offset rel_off,
							 void *bufp,
							 void *v_paramp),
					int (*vectorfn) (int *blocks_p,
							 int count,
							 int blklen,
							 DLOOP_Offset stride,
							 int el_size,
							 DLOOP_Offset rel_off,
							 void *bufp,
							 void *v_paramp),
					void *pieceparams)
{
    int cur_sp, valid_sp;
    DLOOP_Offset last, stream_off;
    struct DLOOP_Dataloop_stackelm *cur_elmp;

    DLOOP_SEGMENT_LOAD_LOCAL_VALUES;

    /* first we ensure that stream_off and first are in the same spot */
    if (first != stream_off) {
	DLOOP_Offset tmp_last;

#ifdef DLOOP_M_VERBOSE
	DLOOP_dbg_printf("first=%d; stream_off=%ld; resetting.\n", first, stream_off);
#endif

	/* TODO: BE SMARTER AND DON'T RESET IF STREAM_OFF IS BEFORE FIRST */
	/* reset to beginning of stream */
	DLOOP_SEGMENT_RESET_VALUES;

	/* TODO: AVOID CALLING MANIP. IF FIRST == 0 */

	/* Note: we're simply using the manipulate function with a NULL piecefn
	 * to handle this case.  It's one more function call, but it dramatically
	 * simplifies this code.
	 */
	tmp_last = first;
	PREPEND_PREFIX(Segment_manipulate)(segp, 0, &tmp_last, NULL, NULL, NULL);
	
	/* verify that we're in the right location */
	if (tmp_last != first) assert(0);

	DLOOP_SEGMENT_LOAD_LOCAL_VALUES;

	/* continue processing... */
#ifdef DLOOP_M_VERBOSE
	DLOOP_dbg_printf("done repositioning stream_off; first=%d, stream_off=%ld, last=%d\n",
		   first, stream_off, last);
#endif
    }

    for (;;) {
#ifdef DLOOP_M_VERBOSE
        DLOOP_dbg_printf("looptop; cur_sp=%d, cur_elmp=%x\n", cur_sp, (unsigned) cur_elmp);
#endif

	if (cur_elmp->loop_p->kind & DLOOP_FINAL_MASK) {
	    int partial_flag, piecefn_indicated_exit;
	    DLOOP_Offset myblocks, basic_size;

	    /* process data region */

	    /* First discover how large a region we *could* process, if it
	     * could all be handled by the processing function.
	     */

	    /* this is the fundamental size at which we should work.
	     */
	    basic_size = cur_elmp->loop_p->el_size;

	    switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
         	case DLOOP_KIND_BLOCKINDEXED:
		case DLOOP_KIND_INDEXED:
		    myblocks = cur_elmp->curblock;
		    break;
		case DLOOP_KIND_VECTOR:
		    /* If we have a vector function and we're at the start of a contiguous
		     * contiguous region, call the vector function on all the blocks.
		     *
		     * Otherwise we'll use the contig function to try to get lined up.
		     */
		    if (vectorfn && cur_elmp->orig_block == cur_elmp->curblock)
			myblocks = cur_elmp->curblock * cur_elmp->curcount;
		    else
			myblocks = cur_elmp->curblock;
		    break;
		default:
		    assert(0);
	    }

#ifdef DLOOP_M_VERBOSE
	    DLOOP_dbg_printf("\thit leaf; cur_sp=%d, elmp=%x, piece_sz=%d\n", cur_sp,
		       (unsigned) cur_elmp, myblocks * basic_size);
#endif

	    /* ??? SHOULD THIS BE >= FOR SOME REASON ??? */
	    if (last != SEGMENT_IGNORE_LAST && stream_off + myblocks * basic_size > last) {
		/* Cannot process the entire "piece" -- round down */
		myblocks = ((last - stream_off) / basic_size);
#ifdef DLOOP_M_VERBOSE
		DLOOP_dbg_printf("\tpartial block count=%d\n", myblocks);
#endif
		if (myblocks == 0) {
#ifdef DLOOP_M_VERBOSE
		    DLOOP_dbg_printf("partial flag and no whole blocks, returning sooner than expected.\n");
#endif
		    DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		    return;
		}
		partial_flag = 1;
	    }
	    else {
		partial_flag = 0; /* handling everything left for this type */
	    }
	    if (vectorfn
		&& (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) == DLOOP_KIND_VECTOR
		&& cur_elmp->curcount > 1
		&& cur_elmp->orig_block == cur_elmp->curblock)
	    {
		piecefn_indicated_exit = vectorfn(&myblocks,
						  cur_elmp->curcount,
						  cur_elmp->orig_block,
						  cur_elmp->loop_p->loop_params.v_t.stride,
						  basic_size, /* for hetero this is a type */
						  cur_elmp->curoffset, /* relative to segp->ptr */
						  segp->ptr, /* start of buffer (from segment) */
						  pieceparams);
	    }
	    else if (contigfn) {
		piecefn_indicated_exit = contigfn(&myblocks,
						  basic_size, /* for hetero this is a type */
						  cur_elmp->curoffset, /* relative to segp->ptr */
						  segp->ptr, /* start of buffer (from segment) */
						  pieceparams);
	    }
	    else {
		piecefn_indicated_exit = 0;
#ifdef DLOOP_M_VERBOSE
		DLOOP_dbg_printf("\tNULL piecefn for this piece\n");
#endif
	    }
	    stream_off += myblocks * basic_size;

	    /* TODO: MAYBE REORGANIZE? */
	    if (myblocks < cur_elmp->curblock) {
		/* Definitely stopping after this.  Either piecefn stopped short or we did due to last param */
		cur_elmp->curoffset += myblocks * basic_size;

		/* NOTE: THIS CODE ASSUMES THAT WE STOP ON WHOLE BASIC SIZES!!! */
		switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
		    case DLOOP_KIND_CONTIG:
		    case DLOOP_KIND_BLOCKINDEXED:
		    case DLOOP_KIND_INDEXED:
			cur_elmp->curblock -= myblocks;
			break;
		    case DLOOP_KIND_VECTOR:
			/* TODO: RECOGNIZE ABILITY TO DO STRIDED COPIES --
			 * ONLY GOOD FOR THE NON-VECTOR CASES...
			 */
			cur_elmp->curblock -= myblocks;
			break;
		}
#ifdef DLOOP_M_VERBOSE
		DLOOP_dbg_printf("partial flag, returning sooner than expected.\n");
#endif
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }

	    /* we at least finished a whole block */

	    /* Update the stack elements.  Either we're done with the count,
	     * in which case it is time to pop off, or we need to reset the
	     * block value (because we just handled an entire block).
	     */
	    if (myblocks > cur_elmp->curblock)
	    {
		/* This can only happen in the case of a vector, so we don't
		 * test for that for performance reasons.
		 */

		/* recall that we only handle more than one contiguous block if
		 * we are at the beginning of a block.  this simplifies the
		 * calculations here.
		 */
		cur_elmp->curcount -= myblocks / cur_elmp->orig_block;
		if (cur_elmp->curcount == 0) DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
		else {
		    /* if we didn't finish the entire type, we need to update the block and offset */
		    cur_elmp->curblock = cur_elmp->orig_block - (myblocks % cur_elmp->orig_block);
		    cur_elmp->curoffset = cur_elmp->orig_offset + (cur_elmp->orig_count - cur_elmp->curcount) *
			cur_elmp->loop_p->loop_params.v_t.stride;
		}
	    }
	    else {
		cur_elmp->curcount--;
		if (cur_elmp->curcount == 0) DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
		else {
		    /* didn't finish with the type. */
		    int count_index;

		    switch (cur_elmp->loop_p->kind & DLOOP_KIND_MASK) {
			/* DLOOP_KIND_CONTIG -- there is always only one block, so we never hit this code */
			case DLOOP_KIND_INDEXED:
			    count_index = cur_elmp->orig_count - cur_elmp->curcount;

			    cur_elmp->orig_block = DLOOP_STACKELM_INDEXED_BLOCKSIZE(cur_elmp, count_index);
			    cur_elmp->curblock   = cur_elmp->orig_block;
			    cur_elmp->curoffset  = cur_elmp->orig_offset + DLOOP_STACKELM_INDEXED_OFFSET(cur_elmp, count_index);
			    break;
			case DLOOP_KIND_VECTOR:
			    cur_elmp->curblock = cur_elmp->orig_block;
			    cur_elmp->curoffset = cur_elmp->orig_offset + (cur_elmp->orig_count - cur_elmp->curcount) *
				cur_elmp->loop_p->loop_params.v_t.stride;
			    break;
			case DLOOP_KIND_BLOCKINDEXED:
			    assert(0);
			    break;
		    }
		}
	    }
	    if (piecefn_indicated_exit) {
		/* The piece function indicated that we should quit processing */
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }
	} /* end of if leaf */
	else if (cur_elmp->curblock == 0) {
	    /* This section is testing to see if we hit the end of the count
	     * for this type (which is not a leaf).  The first test is that we
	     * hit end of block.
	     */
#ifdef DLOOP_M_VERBOSE
	    DLOOP_dbg_printf("\thit end of block; elmp=%x [%d]\n",
			    (unsigned) cur_elmp, cur_sp);
#endif
	    cur_elmp->curcount--;
	    if (cur_elmp->curcount == 0) {
		/* We also hit end of count; pop this type. */
#ifdef DLOOP_M_VERBOSE
		DLOOP_dbg_printf("\talso hit end of count; elmp=%x [%d]\n",
				(unsigned) cur_elmp, cur_sp);
#endif
		DLOOP_SEGMENT_POP_AND_MAYBE_EXIT;
	    }
	    else {
		/* Otherwise we just have a new block.  Reset block value. */
		if ((cur_elmp->loop_p->kind & DLOOP_KIND_MASK) == DLOOP_KIND_INDEXED)
		{
		    /* indexed and struct are the only ones for which this can change
		     * during processing.  and this code doesn't do structs...
		     */
		    cur_elmp->orig_block = DLOOP_STACKELM_INDEXED_BLOCKSIZE(cur_elmp, cur_elmp->orig_count - cur_elmp->curcount);
		}
		cur_elmp->curblock = cur_elmp->orig_block;
		/* TODO: COMBINE INTO NEXT BIG ELSE; WE'RE PROBABLY MAKING AN EXTRA PASS THROUGH OUR LOOPS */
	    }
	} /* end of "hit end of a block, maybe hit end of loop (count)" */
	else {
	    DLOOP_Dataloop_stackelm *next_elmp;
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
	    DLOOP_dbg_printf("\tpushing type, elmp=%x [%d], count=%d, block=%d\n",
			    (unsigned) cur_elmp, cur_sp, count_index, block_index);
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
			block_index * cur_elmp->loop_p->el_extent;
		    break;
		case DLOOP_KIND_VECTOR:
		    /* NOTE: stride is in bytes */
		    next_elmp->orig_offset = cur_elmp->orig_offset +
			count_index * cur_elmp->loop_p->loop_params.v_t.stride +
			block_index * cur_elmp->loop_p->el_extent;
		    break;
		case DLOOP_KIND_BLOCKINDEXED:
		    next_elmp->orig_offset = cur_elmp->orig_offset +
			block_index * cur_elmp->loop_p->el_extent +
			DLOOP_STACKELM_BLOCKINDEXED_OFFSET(cur_elmp, count_index);
		    assert(0);
		    break;
		case DLOOP_KIND_INDEXED:
		    /* Accounting for additional offset from being partway
		     * through a collection of blocks, plus the displacement
		     * for this count.
		     */
		    next_elmp->orig_offset = cur_elmp->orig_offset +
			block_index * cur_elmp->loop_p->el_extent +
			DLOOP_STACKELM_INDEXED_OFFSET(cur_elmp, count_index);
		    break;
	    } /* end of switch */

#ifdef DLOOP_M_VERBOSE
	    DLOOP_dbg_printf("\tstep 1: next orig_offset = %d (0x%x)\n",
			     next_elmp->orig_offset,
			     next_elmp->orig_offset);
#endif

	    /* now we update the curoffset based on the next type */
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
		    next_elmp->curoffset = next_elmp->orig_offset + DLOOP_STACKELM_BLOCKINDEXED_OFFSET(next_elmp, 0);
		    break;
		case DLOOP_KIND_INDEXED:
		    next_elmp->curcount  = next_elmp->orig_count;
		    next_elmp->curblock  = DLOOP_STACKELM_INDEXED_BLOCKSIZE(next_elmp, 0);
		    next_elmp->curoffset = next_elmp->orig_offset + DLOOP_STACKELM_INDEXED_OFFSET(next_elmp, 0);
		    break;
	    } /* end of switch */
	    /* TODO: HANDLE NON-ZERO OFFSETS IN NEXT_ELMP HERE? */

#ifdef DLOOP_M_VERBOSE
	    DLOOP_dbg_printf("\tstep 2: next curoffset = %d (0x%x)\n",
			     next_elmp->curoffset,
			     next_elmp->curoffset);
#endif

	    cur_elmp->curblock--;
	    DLOOP_SEGMENT_PUSH;
	} /* end of else push the datatype */
    } /* end of for (;;) */

#ifdef DLOOP_M_VERBOSE
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
