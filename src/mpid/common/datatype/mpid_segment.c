/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpiimpl.h>
#include <mpid_dataloop.h>

#undef MPID_SP_VERBOSE

/* MPID_Segment_piece_params
 *
 * This structure is used to pass function-specific parameters into our 
 * segment processing function.  This allows us to get additional parameters
 * to the functions it calls without changing the prototype.
 *
 * TODO: MOVE THIS OUT AND MAKE IT USE-DEPENDENT
 */
struct MPID_Segment_piece_params {
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

#define MPIDI_COPY_FROM_VEC(src,dest,stride,type,nelms,count)			\
do {										\
    type * l_src = (type *)src, * l_dest = (type *)dest;                    	\
    int i, j;									\
    const int l_stride = stride;						\
    if (nelms == 1) {								\
        for (i=count;i!=0;i--) {						\
            *l_dest++ = *l_src;							\
            l_src = (type *) ((char *) l_src + l_stride);			\
        }									\
    }										\
    else {									\
        for (i=count; i!=0; i--) {						\
            for (j=0; j<nelms; j++) {						\
                *l_dest++ = l_src[j];						\
	    }									\
            l_src = (type *) ((char *) l_src + l_stride);			\
        }									\
    }                                                                           \
    dest = (char *) l_dest;                                                     \
    src  = (char *) l_src;                                                      \
} while (0)

#define MPIDI_COPY_TO_VEC(src,dest,stride,type,nelms,count)			\
do {										\
    type * l_src = (type *)src, * l_dest = (type *)dest;                    	\
    int i, j;									\
    const int l_stride = stride;						\
    if (nelms == 1) {								\
        for (i=count;i!=0;i--) {						\
            *l_dest = *l_src++;							\
            l_dest = (type *) ((char *) l_dest + l_stride);			\
        }									\
    }										\
    else {									\
        for (i=count; i!=0; i--) {						\
            for (j=0; j<nelms; j++) {						\
                l_dest[j] = *l_src++;						\
	    }									\
            l_dest = (type *) ((char *) l_dest + l_stride);			\
        }									\
    }                                                                           \
    dest = (char *) l_dest;                                                     \
    src  = (char *) l_src;                                                      \
} while (0)


static int MPID_Segment_vector_pack_to_iov(int *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   int basic_size,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_pack_to_iov(int *blocks_p,
					   int el_size,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_contig_unpack_to_buf(int *blocks_p,
					     int el_size,
					     DLOOP_Offset rel_off,
					     void *bufp,
					     void *v_paramp);

static int MPID_Segment_vector_pack_to_buf(int *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   int basic_size,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

static int MPID_Segment_vector_unpack_to_buf(int *blocks_p,
					     int count,
					     int blksz,
					     DLOOP_Offset stride,
					     int basic_size,
					     DLOOP_Offset rel_off,
					     void *bufp,
					     void *v_paramp);

static int MPID_Segment_contig_pack_to_buf(int *blocks_p,
					   int el_size,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp);

/* Segment_pack - we need to implement this if for no other reason
 * than for performance testing
 *
 * Input Parameters:
 * segp - pointer to segment
 * pack_buffer - pointer to buffer to pack into
 * first - first byte index to be packed (or actually packed (?))
 *
 * InOut Parameters:
 * last - pointer to last byte index to be packed plus 1 (makes math easier)
 *
 * This and the similar functions all set up a piece_params structure that
 * they then pass to MPID_Segment_manipulate along with the function that 
 * they want called on each piece.  So in this case MPID_Segment_manipulate
 * will call MPID_Segment_piece_pack() on each piece of the buffer to pack,
 * where a piece is a basic datatype.
 *
 * Eventually we'll probably ditch this approach to gain some speed, but
 * for now it lets me have one function (_manipulate) that implements our
 * algorithm for parsing.
 *
 */
void MPID_Segment_pack(struct DLOOP_Segment *segp,
		       DLOOP_Offset first,
		       DLOOP_Offset *lastp, 
		       void *pack_buffer)
{
    struct MPID_Segment_piece_params pack_params;
    
    pack_params.u.pack.pack_buffer = pack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_pack_to_buf, 
			    MPID_Segment_vector_pack_to_buf,
			    &pack_params);
    return;
}

/* MPID_Segment_pack_vector
 */
void MPID_Segment_pack_vector(struct DLOOP_Segment *segp,
			      DLOOP_Offset first,
			      DLOOP_Offset *lastp,
			      DLOOP_VECTOR *vectorp,
			      DLOOP_Offset *lengthp)
{
    struct MPID_Segment_piece_params packvec_params;

    packvec_params.u.pack_vector.vectorp = vectorp;
    packvec_params.u.pack_vector.index   = 0;
    packvec_params.u.pack_vector.length  = *lengthp;

    assert(*lengthp > 0);

    MPID_Segment_manipulate(segp,
			    first,
			    lastp, 
			    MPID_Segment_contig_pack_to_iov, 
			    MPID_Segment_vector_pack_to_iov,
			    &packvec_params);

    /* last value already handled by MPID_Segment_manipulate */
    *lengthp = packvec_params.u.pack_vector.index;
    return;
}

/* Segment_unpack
 */
void MPID_Segment_unpack(struct DLOOP_Segment *segp,
			 DLOOP_Offset first,
			 DLOOP_Offset *lastp,
			 const DLOOP_Buffer unpack_buffer)
{
    struct MPID_Segment_piece_params unpack_params;
    
    unpack_params.u.unpack.unpack_buffer = (DLOOP_Buffer) unpack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp, 
			    MPID_Segment_contig_unpack_to_buf,
			    MPID_Segment_vector_unpack_to_buf,
			    &unpack_params);
    return;
}

/* MPID_Segment_vector_pack_to_iov
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int MPID_Segment_vector_pack_to_iov(int *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   int basic_size,
					   DLOOP_Offset rel_off, /* offset into buffer */
					   void *bufp, /* start of buffer */
					   void *v_paramp)
{
    int i, size, blocks_left;
    struct MPID_Segment_piece_params *paramp = v_paramp;

    blocks_left = *blocks_p;

    for (i=0; i < count && blocks_left > 0; i++) {
	if (blocks_left > blksz) {
	    size = blksz * basic_size;
	    blocks_left -= blksz;
	}
	else {
	    /* last pass */
	    size = blocks_left * basic_size;
	    blocks_left = 0;
	}

	if (paramp->u.pack_vector.index > 0 && ((char *) bufp + rel_off) ==
	    (((char *) paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_BUF) +
	     paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN))
	{
	    /* add this size to the last vector rather than using up another one */
	    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN += size;
	}
	else {
	    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_BUF = (char *) bufp + rel_off;
	    paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_LEN = size;
	    paramp->u.pack_vector.index++;
	    /* check to see if we have used our entire vector buffer, and if so return 1 to stop processing */
	    if (paramp->u.pack_vector.index == paramp->u.pack_vector.length) {
		/* TODO: FIX SO THAT WE CONTINUE TO DO AGGREGATION ON LAST VECTOR */
		*blocks_p -= blocks_left;
		return 1;
	    }
	}

	rel_off += stride;

    }
    assert(blocks_left == 0);
    return 0;
}


/* MPID_Segment_contig_pack_to_iov
 */
static int MPID_Segment_contig_pack_to_iov(int *blocks_p,
					   int el_size,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int size;
    struct MPID_Segment_piece_params *paramp = v_paramp;

    size = *blocks_p * el_size;

#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[index = %d, loc = (%x + %x) = %x, size = %d]\n",
		    paramp->u.pack_vector.index,
		    (unsigned) bufp,
		    (unsigned) rel_off,
		    (unsigned) bufp + rel_off,
		    size);
#endif
    
    if (paramp->u.pack_vector.index > 0 && ((char *) bufp + rel_off) ==
	(((char *) paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_BUF) +
	 paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN))
    {
	/* add this size to the last vector rather than using up another one */
	paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index - 1].DLOOP_VECTOR_LEN += size;
    }
    else {
	paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_BUF = (char *) bufp + rel_off;
	paramp->u.pack_vector.vectorp[paramp->u.pack_vector.index].DLOOP_VECTOR_LEN = size;
	paramp->u.pack_vector.index++;
	/* check to see if we have used our entire vector buffer, and if so return 1 to stop processing */
	if (paramp->u.pack_vector.index == paramp->u.pack_vector.length) return 1;
    }
    return 0;
}

/* MPID_Segment_unpack_vector
 *
 * Q: Should this be any different from pack vector?
 */
void MPID_Segment_unpack_vector(struct DLOOP_Segment *segp,
				DLOOP_Offset first,
				DLOOP_Offset *lastp,
				DLOOP_VECTOR *vectorp,
				DLOOP_Offset *lengthp)
{
    MPID_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);
    return;
}

/* MPID_Segment_vector_unpack_to_buf
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int MPID_Segment_vector_unpack_to_buf(int *blocks_p,
					     int count,
					     int blksz,
					     DLOOP_Offset stride,
					     int basic_size,
					     DLOOP_Offset rel_off, /* offset into buffer */
					     void *bufp, /* start of buffer */
					     void *v_paramp)
{
    int i, blocks_left, whole_count;
    char *cbufp = (char *) bufp + rel_off;
    struct MPID_Segment_piece_params *paramp = v_paramp;

    whole_count = *blocks_p / blksz;
    blocks_left = *blocks_p % blksz;

    if (basic_size == 4) {
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, stride, int32_t, blksz, whole_count);
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int32_t, blocks_left, 1);
    }
    else if (basic_size == 2) {
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, stride, int16_t, blksz, whole_count);
	MPIDI_COPY_TO_VEC(paramp->u.unpack.unpack_buffer, cbufp, 0, int16_t, blocks_left, 1);
    }
    else {
	for (i=0; i < whole_count; i++) {
	    memcpy(cbufp, paramp->u.unpack.unpack_buffer, blksz * basic_size);
	    paramp->u.unpack.unpack_buffer += blksz * basic_size;
	    cbufp += stride;
	}
	if (blocks_left) {
	    memcpy(paramp->u.unpack.unpack_buffer, cbufp, blocks_left * basic_size);
	    paramp->u.unpack.unpack_buffer += blocks_left * basic_size;
	}
    }
    return 0;
}

/* MPID_Segment_contig_unpack_to_buf
 */
static int MPID_Segment_contig_unpack_to_buf(int *blocks_p,
					     int el_size,
					     DLOOP_Offset rel_off,
					     void *bufp,
					     void *v_paramp)
{
    int size;
    struct MPID_Segment_piece_params *paramp = v_paramp;

    size = *blocks_p * el_size;

#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[h=%x, do=%d, dp=%x, bp=%x, sz=%d]\n", handle, dbufoff, 
	       (unsigned) dbufp, (unsigned) paramp->u.unpack.unpack_buffer, size);
#endif
    
    memcpy((char *) bufp + rel_off, paramp->u.unpack.unpack_buffer, size);
    paramp->u.unpack.unpack_buffer += size;
    return 0;
}


/* MPID_Segment_vector_pack_to_buf
 *
 * Note: this is only called when the starting position is at the beginning
 * of a whole block in a vector type.
 */
static int MPID_Segment_vector_pack_to_buf(int *blocks_p,
					   int count,
					   int blksz,
					   DLOOP_Offset stride,
					   int basic_size,
					   DLOOP_Offset rel_off, /* offset into buffer */
					   void *bufp, /* start of buffer */
					   void *v_paramp)
{
    int i, blocks_left, whole_count;
    char *cbufp = (char *) bufp + rel_off;
    struct MPID_Segment_piece_params *paramp = v_paramp;

    whole_count = *blocks_p / blksz;
    blocks_left = *blocks_p % blksz;

    if (basic_size == 4) {
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, stride, int32_t, blksz, whole_count);
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int32_t, blocks_left, 1);
    }
    else if (basic_size == 2) {
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, stride, int16_t, blksz, whole_count);
	MPIDI_COPY_FROM_VEC(cbufp, paramp->u.pack.pack_buffer, 0, int16_t, blocks_left, 1);
    }
    else {
	for (i=0; i < whole_count; i++) {
	    memcpy(paramp->u.pack.pack_buffer, cbufp, blksz * basic_size);
	    paramp->u.pack.pack_buffer += blksz * basic_size;
	    cbufp += stride;
	}
	if (blocks_left) {
	    memcpy(paramp->u.pack.pack_buffer, cbufp, blocks_left * basic_size);
	    paramp->u.pack.pack_buffer += blocks_left * basic_size;
	}
    }
    return 0;
}


/* MPID_Segment_contig_pack_to_buf
 */
static int MPID_Segment_contig_pack_to_buf(int *blocks_p,
					   int el_size,
					   DLOOP_Offset rel_off,
					   void *bufp,
					   void *v_paramp)
{
    int size;
    struct MPID_Segment_piece_params *paramp = v_paramp;

    size = *blocks_p * el_size;

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * bp = pack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[h=%x, do=%d, dp=%x, bp=%x, sz=%d]\n", handle, rel_off, 
	       (unsigned) bufp, (unsigned) paramp->u.pack.pack_buffer, size);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */

    memcpy(paramp->u.pack.pack_buffer, (char *) bufp + rel_off, size);
    paramp->u.pack.pack_buffer += size;
    return 0;
}










