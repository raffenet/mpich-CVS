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

/* DLOOP_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our 
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 */
struct DLOOP_Segment_piece_params {
    union {
        struct {
            char *pack_buffer;
        } pack;
        struct {
            DLOOP_VECTOR *vectorp;
            int index;
            int length;
        } pack_vector;
        struct {
            char *unpack_buffer;
        } unpack;
        struct {
            int stream_off;
        } print;
    } u;
};

static inline int DLOOP_Stackelm_blocksize(struct DLOOP_Dataloop_stackelm *elmp);

static int DLOOP_Segment_piece_print(DLOOP_Handle handle,
				     int dbufoff, 
				     int size,
				     void *dbufp,
				     struct DLOOP_Segment_piece_params *paramp);
static int DLOOP_Segment_piece_pack(DLOOP_Handle handle,
				    int dbufoff, 
				    int size,
				    void *dbufp,
				    struct DLOOP_Segment_piece_params *paramp);
static int DLOOP_Segment_piece_pack_vector(DLOOP_Handle handle,
					   int dbufoff, 
					   int size,
					   void *dbufp,
					   struct DLOOP_Segment_piece_params *paramp);
static int DLOOP_Segment_piece_unpack(DLOOP_Handle handle,
				      int dbufoff, 
				      int size,
				      void *dbufp,
				      struct DLOOP_Segment_piece_params *paramp);

static void DLOOP_Segment_manipulate(struct DLOOP_Segment *segp,
				     int first, 
				     int *lastp,
				     int (*piecefn)(DLOOP_Handle,
						    int,
						    int,
						    void*,
						    struct DLOOP_Segment_piece_params *), 
				     struct DLOOP_Segment_piece_params *pieceparams);

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
    struct DLOOP_Dataloop_stackelm *elmp;
    struct DLOOP_Dataloop *dlp = 0;
    
    /* first figure out what to do with the datatype/count.
     * there are three cases:
     * - predefined datatype, any count; use the builtin loop only
     * - derived type, count == 1; don't use builtin at all
     * - derived type, count > 1; use builtin for contig of derived type
     */

    if (!DLOOP_Handle_hasloop_macro(handle)) {
	/* simplest case; datatype has no loop, so it cannot have a count. */
	int elmsize;

	DLOOP_Handle_get_size_macro(handle, elmsize);

	/* NOTE: ELMSIZE IS WRONG */
	segp->builtin_loop.kind = DLOOP_KIND_CONTIG | DLOOP_FINAL_MASK 
	    | (elmsize << DLOOP_ELMSIZE_SHIFT);
	segp->builtin_loop.loop_params.c_t.count = count;
	segp->builtin_loop.loop_params.c_t.u.dataloop = 0;
	segp->builtin_loop.el_size = elmsize;
	DLOOP_Handle_get_extent_macro(handle, segp->builtin_loop.el_extent);

	dlp = &segp->builtin_loop;
    }
    else if (count == 1) {
	/* don't use the builtin */
	DLOOP_Handle_get_loopptr_macro(handle, dlp);
    }
    else {
	/* need to use builtin to handle contig; must check loop depth first */
	int depth = 0, elmsize;
	
	DLOOP_Handle_get_loopdepth_macro(handle, depth);
	if (depth >= DLOOP_MAX_DATATYPE_DEPTH) return -1;

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
    
    elmp = &(segp->stackelm[0]);
    /* fill in first stackelm so we don't need a special case in other
     * calls.
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
     */
    elmp->curcount = dlp->loop_params.count;
    elmp->curoffset = 0;
    elmp->loop_p  = dlp;
    elmp->curblock = DLOOP_Stackelm_blocksize(elmp);
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


/* Segment_pack - we need to implement this if for no other reason
 * than for performance testing
 *
 * Input Parameters:
 * segp - pointer to segment
 * pack_buffer - pointer to buffer to pack into
 * first - first byte index to be packed (or actually packed (??))
 *
 * InOut Parameters:
 * last - pointer to last byte index to be packed plus 1 (makes math easier)
 *
 * This and the similar functions all set up a piece_params structure that
 * they then pass to DLOOP_Segment_manipulate along with the function that 
 * they want called on each piece.  So in this case DLOOP_Segment_manipulate
 * will call DLOOP_Segment_piece_pack() on each piece of the buffer to pack,
 * where a piece is a basic datatype.
 *
 * Eventually we'll probably ditch this approach to gain some speed, but
 * for now it lets me have one function (_manipulate) that implements our
 * algorithm for parsing.
 *
 */
void PREPEND_PREFIX(Segment_pack)(struct DLOOP_Segment *segp,
				  int first,
				  int *lastp, 
				  void *pack_buffer)
{
    struct DLOOP_Segment_piece_params pack_params;
    
    pack_params.u.pack.pack_buffer = pack_buffer;
    DLOOP_Segment_manipulate(segp, first, lastp, DLOOP_Segment_piece_pack, 
    &pack_params);
    return;
}

/* MPID_Segment_pack_vector
 */
void PREPEND_PREFIX(Segment_pack_vector)(struct DLOOP_Segment *segp,
					 DLOOP_Offset first,
					 DLOOP_Offset *lastp,
					 DLOOP_VECTOR *vectorp,
					 DLOOP_Offset *lengthp)
{
    struct DLOOP_Segment_piece_params packvec_params;

    packvec_params.u.pack_vector.vectorp = vectorp;
    packvec_params.u.pack_vector.index   = 0;
    packvec_params.u.pack_vector.length  = *lengthp;

    DLOOP_Segment_manipulate(segp, first, lastp, 
                DLOOP_Segment_piece_pack_vector, 
                &packvec_params);

    /* last value already handled by DLOOP_Segment_manipulate */
    *lengthp = packvec_params.u.pack_vector.index;
    return;
}

/* Segment_unpack
 */
void PREPEND_PREFIX(Segment_unpack)(struct DLOOP_Segment *segp,
				    DLOOP_Offset first,
				    DLOOP_Offset *lastp,
				    const DLOOP_Buffer unpack_buffer)
{
    struct DLOOP_Segment_piece_params unpack_params;
    
    unpack_params.u.unpack.unpack_buffer = (DLOOP_Buffer) unpack_buffer;
    DLOOP_Segment_manipulate(segp, first, lastp, 
			    DLOOP_Segment_piece_unpack, 
			    &unpack_params);
    return;
}


/* DLOOP_Segment_piece_pack_vector
 */
static int DLOOP_Segment_piece_pack_vector(DLOOP_Handle handle,
					   int dbufoff, 
					   int size,
					   void *dbufp,
					   struct DLOOP_Segment_piece_params *paramp)
{
#ifdef SP_VERBOSE
    printf("\t[index=%d, loc=%x, size=%d]\n", paramp->u.pack_vector.index,
        (unsigned) dbufp + dbufoff, size);
#endif
    
    /* for now we'll just be stupid about this */
    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_BUF=
        (char*)dbufp + dbufoff;
    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_LEN=
        size;
    
    /* it would be a good idea to aggregate contiguous regions here! */
    
    paramp->u.pack_vector.index++;
    if (paramp->u.pack_vector.index == paramp->u.pack_vector.length) {
        /* we have used up our entire vector buffer; quit */
        return 1;
    }
    else {
        return 0;
    }
}

/* Segment_unpack_vector
 *
 * Q: Should this be any different from pack vector???
 */
void PREPEND_PREFIX(Segment_unpack_vector)(struct DLOOP_Segment *segp,
					   DLOOP_Offset first,
					   DLOOP_Offset *lastp,
					   DLOOP_VECTOR *vectorp,
					   DLOOP_Offset *lengthp)
{
    PREPEND_PREFIX(Segment_pack_vector)(segp, first, lastp, vectorp, lengthp);
    return;
}

/* DLOOP_Segment_piece_unpack
 */
static int DLOOP_Segment_piece_unpack(DLOOP_Handle handle, int dbufoff, 
int size, void *dbufp, struct DLOOP_Segment_piece_params *paramp)
{
#ifdef SU_VERBOSE
    printf("\t[h=%x, do=%d, dp=%x, bp=%x, sz=%d]\n", handle, dbufoff, 
        (unsigned) dbufp, (unsigned) paramp->u.unpack.unpack_buffer, size);
#endif
    
    memcpy((char*)dbufp+dbufoff, paramp->u.unpack.unpack_buffer, size);
    paramp->u.unpack.unpack_buffer += size;
    return 0;
}

/* DLOOP_Segment_piece_pack
 */
static int DLOOP_Segment_piece_pack(DLOOP_Handle handle,
				    int dbufoff,
				    int size, 
				    void *dbufp,
				    struct DLOOP_Segment_piece_params *paramp)
{
    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * bp = pack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef SP_VERBOSE
    printf("\t[h=%x, do=%d, dp=%x, bp=%x, sz=%d]\n", handle, dbufoff, 
        (unsigned) dbufp, (unsigned) paramp->u.pack.pack_buffer, size);
#endif

    memcpy(paramp->u.pack.pack_buffer, (char*)dbufp+dbufoff, size);
    paramp->u.pack.pack_buffer += size;
    return 0;
}

/* DLOOP_Segment_piece_print
 */
static int DLOOP_Segment_piece_print(DLOOP_Handle handle,
				     int dbufoff,
				     int size,
				     void *dbufp,
				     struct DLOOP_Segment_piece_params *paramp)
{
#ifdef D_VERBOSE
    printf("\t[h=%x, do=%d, dp=%x, sz=%d]\n", handle, dbufoff, 
       (unsigned) dbufp, size);
#endif
    return 0;
}



/* DLOOP_Segment_manipulate - do something to a segment
 *
 * This function does all the work, calling the piecefn passed in when it 
 * encounters a datatype element which falls into the range of first..(last-1).
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


static void DLOOP_Segment_manipulate(struct DLOOP_Segment *segp,
				     int first, 
				     int *lastp, 
				     int (*piecefn)(DLOOP_Handle,
						    int,
						    int,
						    void *,
						    struct DLOOP_Segment_piece_params *), 
				     struct DLOOP_Segment_piece_params *pieceparams)
{
    int last;
    int cur_sp, valid_sp, count_index, block_index;
    unsigned long stream_off;
    struct DLOOP_Dataloop_stackelm *elmp, *new_elmp;

    /* if we have parsed past first already, reset to beginning */ 
    if (first < segp->stream_off) {
        
#ifdef M_VERBOSE
        printf("first < stream_off; restarting segment parsing\n");
#endif
        /* segp->ptr should still be ok */
        segp->stream_off = 0;
        segp->cur_sp = 0;
        segp->valid_sp = 0;
        
        elmp = &(segp->stackelm[0]);
        /* elmp->loop_p should still be ok */ 
        elmp->curcount = elmp->loop_p->loop_params.count;
        elmp->curoffset = 0;
        elmp->curblock = DLOOP_Stackelm_blocksize(elmp);
    }

    /* load our local variables from segment, pull in last value */
    last = *lastp;
    cur_sp = segp->cur_sp;
    valid_sp = segp->valid_sp;
    stream_off = segp->stream_off;

    /* parse until we hit the end of the datatype (or exit inside loop) */
    while (cur_sp >= 0) {
        /* I set this here explicitly even though it is set at the end of
         * the loop in order to ensure that I haven't messed anything up.
         *
         * QUESTION: Is this really any faster?  If not, use macro?
         */
        elmp = &(segp->stackelm[cur_sp]);
#ifdef M_VERBOSE
        printf("looptop; cur_sp=%d, elmp=%x\n", cur_sp, (unsigned) elmp);
#endif
        /* Check for "leaf" types, ones that don't have anything 
         * complex underneath.
         *
	 * piece_size is used to keep up with the contiguous region we're 
	 * going to manipulate via our piecefn().
	 *
	 * NOTE: THIS WON'T HANDLE ANYTHING WITH AN EXTENT != SIZE...
         */
	if (elmp->loop_p->kind & DLOOP_FINAL_MASK) {
            int leaf_count;
            unsigned int piece_size, leaf_size;
            int stream_skip = 0;

            leaf_count = elmp->curcount;
            leaf_size = elmp->loop_p->el_size;
            piece_size = leaf_count * leaf_size;

#ifdef M_VERBOSE
            printf("\thit basic; cur_sp=%d, elmp=%x, handle=%x, count=%d\n", 
                   cur_sp, (unsigned) elmp, basic_handle, basic_count);
#endif

            /* here we check to see if we need to call our function piecefn() 
	     * to manipulate the datatype.  things to check:
             * 1) are we completely before "first"?
             * 2) are we crossing the "first" boundary now?
             * 3) are we completely past "first"?
             *
             * 4) are we completely before "last"?
             * 5) are we crossing over "last"?
             */
            if (stream_off + (unsigned long) piece_size 
		<= (unsigned long) first) 
	    {
		/* this part of the datatype isn't in the requested region */
#ifdef M_VERBOSE
                printf("\tskipped basic @ %ld\n", stream_off);
#endif
                
                stream_off += piece_size;
		cur_sp--;
                continue;
            }

            if (stream_off < (unsigned long) first) {
                stream_skip = first - stream_off;
                /* we're going to cross first, so move stream_off forward
                 * and curoffset forward and subtract this region from 
		 * piece_size
                 */
#ifdef M_VERBOSE
                printf("\tskipped part of basic from %ld to %d in stream\n",
                       stream_off, first-1);
#endif

                /* don't change any of the elements of the stackelm,
                 * just local ones for now
                 */
                stream_off = first;
                stream_skip += stream_skip;
                piece_size -= stream_skip;

		/* if stream_skip % basic_size != 0, then we have a problem.
		 *
		 * For now, just report the problem as a warning.
		 */
		if (stream_skip % leaf_size != 0) {
		    fprintf(stderr, 
			    "ROB: warning: requested first (%d) not on basic boundary!\n", 
			    first);
		}
            }

            if (stream_off + (unsigned long)piece_size >= (unsigned long)last) 
	    {
                /* we're going to go past the last requested byte if we read
                 * the whole thing.
                 */
                piece_size = last - stream_off;

		/* assuming we started on a basic boundary, round down to 
		 * an aligned one.
		 */
		piece_size = (piece_size / leaf_size) * leaf_size;
            }

	    if (piece_size == 0) {
		/* region wasn't a whole basic */
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
		return;
	    }

	    /* at this point we've identified a region of the datatype which 
	     * should be aligned as whole basics and which falls into the
	     * range that was requested.
	     */

            if (piecefn((DLOOP_Handle) 0, elmp->curoffset + stream_skip, 
                piece_size, segp->ptr, pieceparams) != 0) 
            {
                /* we got the whole piece, minus perhaps some leading part */
                /* when a piecefn returns nonzero it is indicating that
                 * we should stop processing here.
                 */
                /* this could be optimized with code just below */
#ifdef M_VERBOSE
                printf("\tpiecefn says quit\n");
#endif
                stream_off += stream_skip + piece_size;
		elmp->curcount -= piece_size / leaf_size;
		elmp->curoffset += stream_skip + piece_size;
                if (elmp->curcount == 0) cur_sp--;
		DLOOP_SEGMENT_SAVE_LOCAL_VALUES;
                return;
            }


            /* completed and piecefn says keep going */
            stream_off += stream_skip + piece_size;
	    elmp->curcount -= piece_size / leaf_size;
	    elmp->curoffset += stream_skip + piece_size;
            if (elmp->curcount == 0) cur_sp--;

            continue; /* to keep our if..else depth a little lower */
        }

	assert(0);

        /* this wasn't a leaf */

        /* first determine if we hit an end-of-block, an end-of-count,
         * or neither.
         *
         * in this section we update curcount, curblock, and curoffset
         * so they are ready to be used in push operations.
         */
        if (elmp->curblock == 0) {
#ifdef M_VERBOSE
            printf("\thit end of block; elmp=%x\n", (unsigned) elmp);
#endif
            /* we've hit the end of a block, decrement count and check
             * for done with count too
             */
            elmp->curcount--;
            if (elmp->curcount == 0) {
                /* we also hit end of count */
#ifdef M_VERBOSE
                printf("\thit end of count; elmp=%x; pop!\n", 
                       (unsigned) elmp);
#endif
                cur_sp--;
                continue; /* pop */
            }
            else {
                /* new block: need to reset block count and curoffset */
                elmp->curblock = DLOOP_Stackelm_blocksize(elmp);
            }
        }
        else {
            /* neither end of block nor end of count */
            /* IS THERE ANYTHING TO DO HERE NOW? */
        }

        /* set up local variable to hold pointer to next stackelm */
        new_elmp = &(segp->stackelm[cur_sp+1]);

#ifdef M_VERBOSE
        printf("\tnon-basic: count=%d, blkcnt=%d, curoff=%d\n", 
               elmp->curcount, elmp->curblock, elmp->curoffset);
        printf("\tpush; new elmp=%x\n", (unsigned) new_elmp);
#endif
        /* need to push a datatype.  there are three cases for what to
         * do with respect to setting the dataloop value stored in the 
         * new stackelm, which are based on the current stackelm type
         * and the valid value:
         * struct                   - always load in new
         * non-struct, valid <= cur - load in new
         * non-struct, valid > cur  - use what is there
         *
         * IS THERE ANY WAY TO OPTIMIZE THE CASE WHERE WE ARE RE-PUSHING
         * ON A STACK DUE TO A BLOCKSIZE > 1?
         *
         * SHOULD THIS REALLY BE 2 CASES, NON-STRUCT VALID AND ALL ELSE?
         *
         * At the moment we always assume invalid to keep the size of this 
         * code down a little.
         */
        
        /* count_index and block_index are values that can be used to
         * index into arrays based on the current count and block values
         */
        count_index = elmp->loop_p->loop_params.count - elmp->curcount;
        new_elmp->curoffset = -999999; /* to catch errors */

        if (elmp->loop_p->kind == DLOOP_KIND_STRUCT) {
            block_index = 
                elmp->loop_p->loop_params.s_t.blocksize_array[count_index] -
                elmp->curblock;
#ifdef S_VERBOSE
            printf("\tstruct case\n");
#endif
            valid_sp = cur_sp + 1;

            new_elmp->loop_p = 
                elmp->loop_p->loop_params.s_t.dataloop_array[count_index];
            /* get the offset right */
            new_elmp->curoffset = elmp->curoffset +
                elmp->loop_p->loop_params.s_t.offset_array[count_index] +
                block_index * new_elmp->loop_p->el_extent;
#ifdef S_VERBOSE
            printf("struct: curoff=%d, offpart=%d, extpart=%d\n",
                   elmp->curoffset, 
                   elmp->loop_p->loop_params.s_t.offset_array[count_index], 
                   block_index * new_elmp->loop_p->el_extent);
#endif
        }
#if 0
        /* else if (valid_sp <= cur_sp) { */
#else
        /* for now don't do the don't copy optimization */
        else if (1) {
#endif
            struct DLOOP_Dataloop_stackelm *new_elmp = &(segp->stackelm[cur_sp+1]);

#ifdef S_VERBOSE
            printf("\tnon-struct invalid case\n");
#endif

            valid_sp = cur_sp + 1;

            /* need to initialize next stackelm offset and loop_p.
             * count handled later.
             * 
             * here we need a switch because the pointers to the 
             * dataloops aren't in the same places in all these.
             *
             * I think this is also the right place to update the
             * curoffset value that is used for vectors, especially
             * since we've already done the switch on the kind.
             */
            switch(elmp->loop_p->kind) {
            case DLOOP_KIND_CONTIG:
                /* no blocksize for contig */
                new_elmp->loop_p = 
                    elmp->loop_p->loop_params.c_t.u.dataloop;
                new_elmp->curoffset = elmp->curoffset +
                    count_index * new_elmp->loop_p->el_extent;
                break;
            case DLOOP_KIND_VECTOR:
                block_index = elmp->loop_p->loop_params.v_t.blocksize - 
                    elmp->curblock;
                new_elmp->loop_p = 
                    elmp->loop_p->loop_params.v_t.u.dataloop;
                new_elmp->curoffset = elmp->curoffset +
                    count_index * elmp->loop_p->loop_params.v_t.stride +
                    block_index * new_elmp->loop_p->el_extent;
                break;
            case DLOOP_KIND_BLOCKINDEXED:
                block_index = elmp->loop_p->loop_params.bi_t.blocksize - 
                    elmp->curblock;
                new_elmp->loop_p = 
                    elmp->loop_p->loop_params.bi_t.u.dataloop;
                new_elmp->curoffset = elmp->curoffset +
                    elmp->loop_p->loop_params.bi_t.offset_array[count_index] +
                    block_index * new_elmp->loop_p->el_extent;
                break;
            case DLOOP_KIND_INDEXED:
                block_index = 
                    elmp->loop_p->loop_params.i_t.blocksize_array[count_index]-
                    elmp->curblock;
                new_elmp->loop_p = 
                    elmp->loop_p->loop_params.i_t.u.dataloop;
                new_elmp->curoffset = elmp->curoffset +
                    elmp->loop_p->loop_params.i_t.offset_array[count_index] +
                    block_index * new_elmp->loop_p->el_extent;
                break;
#if 0
            case DLOOP_KIND_BASIC:
                break;
#endif
            default:
                /* ERROR!!!  SHOULDN'T EVER GET HERE!!! */
                break;
            }
        }
        else {
            /* just fall through for the third case (use what is there) */
#ifdef S_VERBOSE
            printf("\tnon-struct valid case\n");
#endif
        }
        /* decrement block count on this element, must be done after
         * the new stackelm calculations
         */
        elmp->curblock--;
        /* increment cur_sp, then update count and offset for all cases */
        cur_sp++;

        /* set elmp here; could move set at top out of while loop to
         * go a little bit faster
         */
        elmp = &(segp->stackelm[cur_sp]);
        elmp->curcount = elmp->loop_p->loop_params.count;
        elmp->curblock = DLOOP_Stackelm_blocksize(elmp);
	/* elmp->curoffset was set up above */
    }
    
    /* exited while loop, save state */
#ifdef M_VERBOSE
    printf("hit end of datatype\n");
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
       
    switch(dlp->kind) {
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
#if 0
    case DLOOP_KIND_BASIC:
#endif
    default:
        return 1; /* there can be only one */
        break;
    }
}



/* 
 * Local variables:
 * c-indent-tabs-mode: nil
 * End:
 */

