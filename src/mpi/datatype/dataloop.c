/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "dataloop.h"

/* Maximum depth of any dataloop stack */
#define MAX_DATALOOP_STK 16

/*
  Simple pack routine, using a dataloop description.
 */

#ifdef FOO
#define VEC_COPY(src,dest,stride,type,nelms,count) \
{ type *restrict src_t = (type *)src, * restrict dest_t = (type *)dest; int i, j;\
  const int l_stride = stride;\
  if (nelms == 1) {\
      for (i=count;i!=0;i--) {\
          *dest_t++ = *src_t; src_t += l_stride;}}\
  else {\
      for (i=count; i!=0;i--) {\
          for (j=0; j<nelms; j++) {\
              *dest_t++ = src_t[j];} src_t += l_stride;}}\
  dest=(char *)dest_t;}
#else
#define VEC_COPY(src,dest,stride,type,nelms,count) \
{ type *restrict src_t = (type *)src, * restrict dest_t = (type *)dest; int i, j;\
  if (nelms == 1) {\
      for (i=count;i!=0;i--) {\
          *dest_t++ = *src_t; src_t += stride;}}\
  else {\
      for (i=count; i!=0;i--) {\
          for (j=0; j<nelms; j++) {\
              *dest_t++ = src_t[j];} src_t += stride;}}\
  dest=(char *)dest_t;}
#endif
/* This version should be used when the entire loop fits into the destination
   buffer.  This minimizes the amount of extra processing.

   To avoid copying the loopinfo in the case that there is a only a single
   loopinfo (e.g., the stack never has a depth greater than one), we keep
   a separate curloopinfo and only save the data in loopinfo if we push the
   value onto the stack.  This should help reduce the overhead of this routine
   for small counts.

   ***THE UPDATE OF CURLOOPINFO MAY NOT BE CORRECT YET.  
*/
void MPID_Segment_pack( MPID_Dataloop *loopinfo, 
			char * restrict src_buf, char * restrict dest_buf )
{
    int cur_sp = 0, valid_sp = 0;
    MPID_Dataloop_stackelm stackelm[MAX_DATALOOP_STK], * restrict curstackelm;
    MPID_Dataloop * restrict curloopinfo = loopinfo;
    int kind;

    curstackelm = &stackelm[0];
    curstackelm->curcount = 0;

    do {
	kind = curloopinfo->kind;
	if (kind & DATALOOP_FINAL_MASK) {
	    /* This is a simple datatype, such as a vector of 
	       simple elements or a contiguous block loop */
	    int      i, nbytes, stride, count, elmsize;
	    char     *restrict sbuf; /* temporary for loops that update src buf */
	    MPI_Aint *offset_array;
	    int      *nbytes_array;
	    switch (kind & DATALOOP_KIND_MASK) {
	    case MPID_DTYPE_CONTIG:
		nbytes = curloopinfo->loop_params.c_t.count;
		memcpy( dest_buf, src_buf, nbytes );
		dest_buf += nbytes;
		break;
	    case MPID_DTYPE_VECTOR:
		elmsize   = curloopinfo->kind >> DATALOOP_ELMSIZE_SHIFT;
		count     = curloopinfo->loop_params.v_t.count;
		nbytes    = curloopinfo->loop_params.v_t.blocksize;
		stride    = curloopinfo->loop_params.v_t.stride;
		if (elmsize == 4) {
		    VEC_COPY(src_buf,dest_buf,stride,int32_t,nbytes,count);
		}
		else if (elmsize == 2) {
		    VEC_COPY(src_buf,dest_buf,stride,int16_t,nbytes,count);
		}
		else {
		    sbuf      = src_buf;
		    for (i=0; i<count; i++) {
			memcpy( dest_buf, sbuf, nbytes );
			dest_buf += nbytes;
			sbuf += stride;
		    }
		}
		/* final sbuf update is extent */
		src_buf += curloopinfo->extent;
		break;
	    case MPID_DTYPE_BLOCKINDEXED:
		count     = curloopinfo->loop_params.bi_t.count;
		nbytes    = curloopinfo->loop_params.bi_t.blocksize;
		offset_array = curloopinfo->loop_params.bi_t.offset;
		for (i=0; i<count; i++) {
		    memcpy( dest_buf, src_buf + offset_array[i], nbytes );
		    dest_buf += nbytes;
		}
		break;
	    case MPID_DTYPE_INDEXED:
		count     = curloopinfo->loop_params.i_t.count;
		nbytes_array = curloopinfo->loop_params.i_t.blocksize;
		offset_array = curloopinfo->loop_params.i_t.offset;
		for (i=0; i<count; i++) {
		    nbytes = nbytes_array[i];
		    memcpy( dest_buf, src_buf + offset_array[i], nbytes );
		    dest_buf += nbytes;
		}
		break;
	    case MPID_DTYPE_STRUCT:
		/* Should never happen; for optimized homogenous code, 
		   STRUCT is the same as INDEXED */
		break;
	    }
	    src_buf += curloopinfo->extent;
	    cur_sp--;
	    curstackelm--;
	    curloopinfo = &curstackelm->loopinfo;
	} else if (curstackelm->curcount ==
		   curloopinfo->loop_params.count) {
	    /* We are done with this datatype */
	    cur_sp--;
	    curstackelm--;
	    curloopinfo = &curstackelm->loopinfo;
	}
	else {
	    /* We need to push a datatype.  Two cases: struct or other.
	       Note that since we know that this is not a leaf type, we do
	       not need to mask of the leaf bit */
	    if (kind == MPID_DTYPE_STRUCT) {
		/* Get the next struct type and push it */
		stackelm[cur_sp+1].loopinfo = 
		    (curloopinfo->loop_params.s_t.dataloop[curstackelm->curcount]);
	    }
	    else {
		/* Optimization: We don't need to copy a stack element twice */
		if (valid_sp <= cur_sp) {
		    stackelm[cur_sp].loopinfo = *curloopinfo;
		    stackelm[cur_sp+1].loopinfo = 
			*curloopinfo->loop_params.cm_t.dataloop;
		    valid_sp = cur_sp + 1;
		}
	    }
	    curstackelm->curcount ++;
	    cur_sp++;
	    curstackelm++;
	    curstackelm->curcount = 0;
	    curloopinfo = &curstackelm->loopinfo;
	}
	if (cur_sp < 0) break; /* Exit from loop */
	/* Otherwise, prepare for the next iteration */
	curloopinfo = &curstackelm->loopinfo;
    } while(1);/* No while since the test is within the loop */
}




