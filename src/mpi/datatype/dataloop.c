
/*
  
 */
#include "mpiimpl.h"
#include "dataloop.h"

/*
  Simple pack routine, using a dataloop description.
 */

/* This version should be used when the entire loop fits into the destination
   buffer.  This minimizes the amount of extra processing 

   
*/
void MPID_Segment_pack( MPID_Dataloop *loopinfo, 
			char *src_buf, char *dest_buf )
{
    int cur_sp = 0, valid_sp = 0;
    MPID_Dataloop_stackelm stackelm[16];

    stackelm[cur_sp].loopinfo  = *loopinfo;
    stackelm[cur_sp].curcount = 0;

    while (cur_sp >= 0) {
	if (stackelm[cur_sp].loopinfo.kind & DATALOOP_FINAL_MASK) {
	    /* This is a simple datatype, such as a vector of 
	       simple elements or a contiguous block loop */
	    int      i, nbytes, stride, count;
	    char     *sbuf; /* temporary for loops that update src buf */
	    MPI_Aint *offset_array;
	    int      *nbytes_array;
	    switch (stackelm[cur_sp].loopinfo.kind & ~DATALOOP_FINAL_MASK) {
	    case MPID_CONTIG:
		nbytes = stackelm[cur_sp].loopinfo.loop_params.c_t.count;
		memcpy( dest_buf, src_buf, nbytes );
		dest_buf += nbytes;
		break;
	    case MPID_VECTOR:
		count     = stackelm[cur_sp].loopinfo.loop_params.v_t.count;
		nbytes    = stackelm[cur_sp].loopinfo.loop_params.v_t.blocksize;
		stride    = stackelm[cur_sp].loopinfo.loop_params.v_t.stride;
		sbuf      = src_buf;
		if ((nbytes & 0x3) == 0) {
		    int * int_dbuf = (int *)dest_buf;
		    int * int_sbuf = (int *)sbuf;
		    int int_nbytes = nbytes >> 2;
		    int int_stride = stride >> 2;
		    int j;
		    if (int_nbytes == 1) {
			for (i=0; i<count; i++) {
			    *int_dbuf++ = *int_sbuf;
			    int_sbuf += int_stride;
			}
		    }
		    else {
			for (i=0; i<count; i++) {
			    for (j=0; j<int_nbytes; j++) {
				*int_dbuf++ = int_sbuf[j];
			    }
			    int_sbuf += int_stride;
			}
		    }
		    dest_buf = (char *)int_dbuf;
		    sbuf     = (char *)int_sbuf;
		}
		else {
		    for (i=0; i<count; i++) {
			memcpy( dest_buf, sbuf, nbytes );
			dest_buf += nbytes;
			sbuf += stride;
		    }
		}
		break;
	    case MPID_BLOCKINDEXED:
		nbytes    = stackelm[cur_sp].loopinfo.loop_params.bi_t.blocksize;
		offset_array = stackelm[cur_sp].loopinfo.loop_params.bi_t.offset;
		for (i=0; i<count; i++) {
		    memcpy( dest_buf, src_buf + offset_array[i], nbytes );
		    dest_buf += nbytes;
		}
		break;
	    case MPID_INDEXED:
		nbytes_array = stackelm[cur_sp].loopinfo.loop_params.i_t.blocksize;
		offset_array = stackelm[cur_sp].loopinfo.loop_params.i_t.offset;
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
	    src_buf += stackelm[cur_sp].loopinfo.extent;
	    cur_sp--;
	} else if (stackelm[cur_sp].curcount ==
		   stackelm[cur_sp].loopinfo.loop_params.count) {
	    /* We are done with this datatype */
	    cur_sp--;
	}
	else {
	    /* We need to push a datatype.  Two cases: struct or other.
	       Note that since we know that this is not a leaf type, we do
	       not need to mask of the leaf bit */
#ifdef FOO
	    if (stackelm[cur_sp].loopinfo.kind == MPID_STRUCT) {
		/* Get the next struct type and push it */
		stackelm[cur_sp+1].loopinfo = 
		    *(stackelm[cur_sp].loopinfo.loop_params.s_t.datatype[stackelm[cur_sp].curcount]);
	    }
	    else {
		/* Optimization: We don't need to copy a stack element twice */
		if (valid_sp <= cur_sp) {
		    stackelm[cur_sp+1].loopinfo = 
			*stackelm[cur_sp].loopinfo.loop_params.cm_t.datatype;
		    valid_sp = cur_sp + 1;
		}
	    }
#endif
	    stackelm[cur_sp].curcount ++;
	    cur_sp++;
	    stackelm[cur_sp].curcount = 0;
	}
    }
}
