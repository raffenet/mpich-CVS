
/*
  
 */
#include "mpiimpl.h"
#include "dataloop.h"

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
   buffer.  This minimizes the amount of extra processing 
*/
void MPID_Segment_pack( MPID_Dataloop *loopinfo, 
			char * restrict src_buf, char * restrict dest_buf )
{
    int cur_sp = 0, valid_sp = 0;
    MPID_Dataloop_stackelm stackelm[16], * restrict curstackelm;
    int kind;

    curstackelm = &stackelm[0];
    curstackelm->loopinfo = *loopinfo;
    curstackelm->curcount = 0;

    do {
	kind = curstackelm->loopinfo.kind;
	if (kind & DATALOOP_FINAL_MASK) {
	    /* This is a simple datatype, such as a vector of 
	       simple elements or a contiguous block loop */
	    int      i, nbytes, stride, count, elmsize;
	    char     *restrict sbuf; /* temporary for loops that update src buf */
	    MPI_Aint *offset_array;
	    int      *nbytes_array;
	    switch (kind & DATALOOP_KIND_MASK) {
	    case MPID_CONTIG:
		nbytes = curstackelm->loopinfo.loop_params.c_t.count;
		memcpy( dest_buf, src_buf, nbytes );
		dest_buf += nbytes;
		break;
	    case MPID_VECTOR:
		elmsize   = curstackelm->loopinfo.kind >> DATALOOP_ELMSIZE_SHIFT;
		count     = curstackelm->loopinfo.loop_params.v_t.count;
		nbytes    = curstackelm->loopinfo.loop_params.v_t.blocksize;
		stride    = curstackelm->loopinfo.loop_params.v_t.stride;
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
		src_buf += curstackelm->loopinfo.extent;
		break;
	    case MPID_BLOCKINDEXED:
		count     = curstackelm->loopinfo.loop_params.bi_t.count;
		nbytes    = curstackelm->loopinfo.loop_params.bi_t.blocksize;
		offset_array = curstackelm->loopinfo.loop_params.bi_t.offset;
		for (i=0; i<count; i++) {
		    memcpy( dest_buf, src_buf + offset_array[i], nbytes );
		    dest_buf += nbytes;
		}
		break;
	    case MPID_INDEXED:
		count     = curstackelm->loopinfo.loop_params.i_t.count;
		nbytes_array = curstackelm->loopinfo.loop_params.i_t.blocksize;
		offset_array = curstackelm->loopinfo.loop_params.i_t.offset;
		for (i=0; i<count; i++) {
		    nbytes = nbytes_array[i];
		    memcpy( dest_buf, src_buf + offset_array[i], nbytes );
		    dest_buf += nbytes;
		}
		break;
	    case MPID_STRUCT:
		/* Should never happen; for optimized homogenous code, 
		   STRUCT is the same as INDEXED */
		break;
	    }
	    src_buf += curstackelm->loopinfo.extent;
	    cur_sp--;
	    curstackelm--;
	} else if (curstackelm->curcount ==
		   curstackelm->loopinfo.loop_params.count) {
	    /* We are done with this datatype */
	    cur_sp--;
	    curstackelm--;
	}
	else {
	    /* We need to push a datatype.  Two cases: struct or other.
	       Note that since we know that this is not a leaf type, we do
	       not need to mask of the leaf bit */
	    if (kind == MPID_STRUCT) {
		/* Get the next struct type and push it */
		stackelm[cur_sp+1].loopinfo = 
		    (curstackelm->loopinfo.loop_params.s_t.dataloop[curstackelm->curcount]);
	    }
	    else {
		/* Optimization: We don't need to copy a stack element twice */
		if (valid_sp <= cur_sp) {
		    stackelm[cur_sp+1].loopinfo = 
			*curstackelm->loopinfo.loop_params.cm_t.dataloop;
		    valid_sp = cur_sp + 1;
		}
	    }
	    curstackelm->curcount ++;
	    cur_sp++;
	    curstackelm++;
	    curstackelm->curcount = 0;
	}
    } while (cur_sp >= 0);
}
