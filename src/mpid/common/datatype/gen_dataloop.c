/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <string.h>
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

#undef DEBUG_DLOOP_SIZE

/* Dataloops
 *
 * The functions here are used for the creation, copying, update, and display
 * of DLOOP_Dataloop structures and trees of these structures.
 *
 * Currently we store trees of dataloops in contiguous regions of memory.  They
 * are stored in such a way that subtrees are also stored contiguously.  This
 * makes it somewhat easier to copy these subtrees around.  Keep this in mind
 * when looking at the functions below.
 *
 * The structures used in this file are defined in mpid_datatype.h.  There is
 * no separate mpid_dataloop.h at this time.
 *
 * OPTIMIZATIONS:
 *
 * There are spots in the code with OPT tags that indicate where we could 
 * optimize particular calculations or avoid certain checks.
 *
 * NOTES:
 *
 * Just using malloc/free right now; probably need to fix later.
 *
 * Don't have locks in place at this time!
 */

/*@
  Dataloop_alloc - allocate the resources used to store a dataloop

  Input Parameters:
. none
@*/
struct DLOOP_Dataloop * PREPEND_PREFIX(Dataloop_alloc)(void)
{
    return DLOOP_Malloc(sizeof(struct DLOOP_Dataloop));
}

/*@
  Dataloop_free - deallocate the resources used to store a dataloop

  Input Parameters:
. dataloop - pointer to dataloop structure
@*/
void PREPEND_PREFIX(Dataloop_free)(struct DLOOP_Dataloop *dataloop)
{
    memset(dataloop, 0, sizeof(DLOOP_Dataloop_common));
    DLOOP_Free(dataloop);
    return;
}
/*@
  Dataloop_copy - Copy an arbitrary dataloop structure, updating
  pointers as necessary

  Input Parameters:
+ dest   - pointer to destination region
. src    - pointer to original dataloop structure
- size   - size of dataloop structure

  This routine parses the dataloop structure as it goes in order to
  determine what exactly it needs to update.

  Notes:
  It assumes that the source dataloop was allocated in our usual way;
  this means that the entire dataloop is in a contiguous region and that
  the root of the tree is first in the array.

  This has some implications:
+ we can use a contiguous copy mechanism to copy the majority of the
  structure
- all pointers in the region are relative to the start of the data region
  the first dataloop in the array is the root of the tree
@*/
void PREPEND_PREFIX(Dataloop_copy)(void *dest,
				   void *src,
				   int size)
{
    DLOOP_Offset ptrdiff;

    /* copy region first */
    memcpy(dest, src, size);

    /* calculate difference in pointer values */
    /* NEED TO MAKE SURE THAT AN INT CAN STORE THE DIFF */
    ptrdiff = (char *)dest - (char *)src;

    /* traverse structure updating pointers */
    PREPEND_PREFIX(Dataloop_update)(dest, ptrdiff);

    return;
}

/*@
  Dataloop_size - return the size of the data described by the dataloop

  Input Parameters:
+ dl_p   - pointer to dataloop for which we will return the size
- sizefn - function for determining size of types in the corresponding stream
           (passing NULL will instead result in el_size values being used)

@*/
DLOOP_Offset
PREPEND_PREFIX(Dataloop_stream_size)(struct DLOOP_Dataloop *dl_p,
				     DLOOP_Offset (*sizefn)(DLOOP_Type el_type))
{
    DLOOP_Offset ret;

    if ((dl_p->kind & DLOOP_KIND_MASK) == DLOOP_KIND_STRUCT) {
	int i;

	ret = 0;
	for (i=0; i < dl_p->loop_params.s_t.count; i++) {
	    ret += dl_p->loop_params.s_t.blocksize_array[i] *
		PREPEND_PREFIX(Dataloop_stream_size)(dl_p->loop_params.s_t.dataloop_array[i],
						     sizefn);
	}
    }
    else {
	DLOOP_Offset tmp_sz, tmp_ct = 1;

	for (;;) {
	    switch (dl_p->kind & DLOOP_KIND_MASK) {
		case DLOOP_KIND_CONTIG:
		    tmp_ct *= dl_p->loop_params.c_t.count;
#ifdef DEBUG_DLOOP_SIZE
		    DLOOP_dbg_printf("stream_size: contig: ct = %d; new tot_ct = %d\n",
				     dl_p->loop_params.c_t.count, tmp_ct);
#endif
		    break;
		case DLOOP_KIND_VECTOR:
		    tmp_ct *=
			dl_p->loop_params.v_t.count * dl_p->loop_params.v_t.blocksize;
#ifdef DEBUG_DLOOP_SIZE
		    DLOOP_dbg_printf("stream_size: vector: ct = %d; blk = %d; new tot_ct = %d\n",
				     dl_p->loop_params.v_t.count,
				     dl_p->loop_params.v_t.blocksize,
				     tmp_ct);
#endif
		    break;
		case DLOOP_KIND_BLOCKINDEXED:
		    tmp_ct *= dl_p->loop_params.bi_t.total_blocks;
#ifdef DEBUG_DLOOP_SIZE
		    DLOOP_dbg_printf("stream_size: blkindexed: blks = %d; new tot_ct = %d\n",
				     dl_p->loop_params.bi_t.total_blocks,
				     tmp_ct);
#endif
		    break;
		case DLOOP_KIND_INDEXED:
		    tmp_ct *= dl_p->loop_params.i_t.total_blocks;
#ifdef DEBUG_DLOOP_SIZE
		    DLOOP_dbg_printf("stream_size: contig: blks = %d; new tot_ct = %d\n",
				     dl_p->loop_params.i_t.total_blocks,
				     tmp_ct);
#endif
		break;
		default:
		    assert(0);
		    break;
	    }

	    if (dl_p->kind & DLOOP_FINAL_MASK) break;
	    else {
		assert(dl_p->loop_params.cm_t.dataloop != NULL);
		dl_p = dl_p->loop_params.cm_t.dataloop;
	    }
	}

	/* call fn for size using bottom type, or use size if fnptr is NULL */
	tmp_sz = ((sizefn) ? sizefn(dl_p->el_type) : dl_p->el_size);

	ret = tmp_sz * tmp_ct;
    }

    return ret;
}

/*@
  Dataloop_update - update pointers after a copy operation

  Input Parameters:
+ dataloop - pointer to loop to update
- ptrdiff - value indicating offset between old and new pointer values

  This function is used to recursively update all the pointers in a
  dataloop tree.
@*/
void PREPEND_PREFIX(Dataloop_update)(struct DLOOP_Dataloop *dataloop,
				     DLOOP_Offset ptrdiff)
{
    /* OPT: only declare these variables down in the Struct case */
    int i;
    struct DLOOP_Dataloop **looparray;

    switch(dataloop->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_CONTIG:
	case DLOOP_KIND_VECTOR:
	    /*
	     * All these really ugly assignments are really of the form:
	     *
	     * ((char *) dataloop->loop_params.c_t.loop) += ptrdiff;
	     *
	     * However, some compilers spit out warnings about casting on the
	     * LHS, so we get this much nastier form instead (using common
	     * struct for contig and vector): 
	     */
	    dataloop->loop_params.cm_t.dataloop = (struct DLOOP_Dataloop *) 
		((char *) dataloop->loop_params.cm_t.dataloop + ptrdiff);

	    if (!(dataloop->kind & DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_update)(dataloop->loop_params.cm_t.dataloop, ptrdiff);
	    break;

	case DLOOP_KIND_BLOCKINDEXED:
	    dataloop->loop_params.bi_t.offset_array = (DLOOP_Offset *)
		((char *) dataloop->loop_params.bi_t.offset_array + ptrdiff);
	    dataloop->loop_params.bi_t.dataloop = (struct DLOOP_Dataloop *)
		((char *) dataloop->loop_params.bi_t.dataloop + ptrdiff);

	    if (!(dataloop->kind & DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_update)(dataloop->loop_params.bi_t.dataloop, ptrdiff);
	    break;

	case DLOOP_KIND_INDEXED:
	    dataloop->loop_params.i_t.blocksize_array = (int *)
		((char *) dataloop->loop_params.i_t.blocksize_array + ptrdiff);
	    dataloop->loop_params.i_t.offset_array = (DLOOP_Offset *)
		((char *) dataloop->loop_params.i_t.offset_array + ptrdiff);
	    dataloop->loop_params.i_t.dataloop = (struct DLOOP_Dataloop *)
		((char *) dataloop->loop_params.i_t.dataloop + ptrdiff);

	    if (!(dataloop->kind & DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_update)(dataloop->loop_params.i_t.dataloop, ptrdiff);
	    break;

	case DLOOP_KIND_STRUCT:
	    dataloop->loop_params.s_t.blocksize_array = (int *)
		((char *) dataloop->loop_params.s_t.blocksize_array + ptrdiff);
	    dataloop->loop_params.s_t.offset_array = (DLOOP_Offset *)
		((char *) dataloop->loop_params.s_t.offset_array + ptrdiff);
	    dataloop->loop_params.s_t.dataloop_array = (struct DLOOP_Dataloop **)
		((char *) dataloop->loop_params.s_t.dataloop_array + ptrdiff);

	    /* fix the N dataloop pointers too */
	    looparray = dataloop->loop_params.s_t.dataloop_array;
	    for (i=0; i < dataloop->loop_params.s_t.count; i++) {
		looparray[i] = (struct DLOOP_Dataloop *)
		    ((char *) looparray[i] + ptrdiff);
	    }

	    if (dataloop->kind & DLOOP_FINAL_MASK) break;

	    for (i=0; i < dataloop->loop_params.s_t.count; i++) {
		PREPEND_PREFIX(Dataloop_update)(looparray[i], ptrdiff);
	    }
	    break;
	default:
	    assert(0);
	    break;
    }
    return;
}

/*@
  Dataloop_print - dump a dataloop tree to stdout for debugging
  purposes

  Input Parameters:
+ dataloop - root of tree to dump
- depth - starting depth; used to help keep up with where we are in the tree
@*/
void PREPEND_PREFIX(Dataloop_print)(struct DLOOP_Dataloop *dataloop,
				    int depth)
{
    int i;

    DLOOP_dbg_printf("loc=%x, treedepth=%d, kind=%d, el_extent=%d\n",
		     (int) dataloop, depth, dataloop->kind, dataloop->el_extent);
    switch(dataloop->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_CONTIG:
	    DLOOP_dbg_printf("\tcount=%d, datatype=%x\n", 
			     dataloop->loop_params.c_t.count, 
			     (int) dataloop->loop_params.c_t.dataloop);
	    if (!(dataloop->kind | DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_print)(dataloop->loop_params.c_t.dataloop, depth+1);
	    break;
	case DLOOP_KIND_VECTOR:
	    DLOOP_dbg_printf("\tcount=%d, blksz=%d, stride=%d, datatype=%x\n",
			     dataloop->loop_params.v_t.count, 
			     dataloop->loop_params.v_t.blocksize, 
			     dataloop->loop_params.v_t.stride,
			     (int) dataloop->loop_params.v_t.dataloop);
	    if (!(dataloop->kind | DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_print)(dataloop->loop_params.v_t.dataloop, depth+1);
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    DLOOP_dbg_printf("\tcount=%d, blksz=%d, datatype=%x\n",
			     dataloop->loop_params.bi_t.count, 
			     dataloop->loop_params.bi_t.blocksize, 
			     (int) dataloop->loop_params.bi_t.dataloop);
	    /* print out offsets later */
	    if (!(dataloop->kind | DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_print)(dataloop->loop_params.bi_t.dataloop, depth+1);
	    break;
	case DLOOP_KIND_INDEXED:
	    DLOOP_dbg_printf("\tcount=%d, datatype=%x\n",
			     dataloop->loop_params.i_t.count,
			     (int) dataloop->loop_params.i_t.dataloop);
	    /* print out blocksizes and offsets later */
	    if (!(dataloop->kind | DLOOP_FINAL_MASK))
		PREPEND_PREFIX(Dataloop_print)(dataloop->loop_params.i_t.dataloop, depth+1);
	    break;
	case DLOOP_KIND_STRUCT:
	    DLOOP_dbg_printf("\tcount=%d\n\tblocksizes: ", dataloop->loop_params.s_t.count);
	    for (i=0; i < dataloop->loop_params.s_t.count; i++)
		DLOOP_dbg_printf("%d ", dataloop->loop_params.s_t.blocksize_array[i]);
	    DLOOP_dbg_printf("\n\toffsets: ");
	    for (i=0; i < dataloop->loop_params.s_t.count; i++)
		DLOOP_dbg_printf("%d ", dataloop->loop_params.s_t.offset_array[i]);
	    DLOOP_dbg_printf("\n\tdatatypes: ");
	    for (i=0; i < dataloop->loop_params.s_t.count; i++)
		DLOOP_dbg_printf("%x ", (int) dataloop->loop_params.s_t.dataloop_array[i]);
	    DLOOP_dbg_printf("\n");
	    if (dataloop->kind | DLOOP_FINAL_MASK) break;

	    for (i=0; i < dataloop->loop_params.s_t.count; i++) {
		PREPEND_PREFIX(Dataloop_print)(dataloop->loop_params.s_t.dataloop_array[i],depth+1);
	    }
	    break;
	default:
	    assert(0);
	    break;
    }
    return;
}
