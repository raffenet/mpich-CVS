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

#if 0
static int MPID_Segment_piece_print(DLOOP_Handle handle,
				    DLOOP_Offset dbufoff, 
				    int size,
				    void *dbufp,
				    void *paramp);
#endif
static int MPID_Segment_piece_pack(DLOOP_Handle handle,
				   DLOOP_Offset dbufoff, 
				   int size,
				   void *dbufp,
				   void *paramp);
static int MPID_Segment_piece_pack_vector(DLOOP_Handle handle,
					  DLOOP_Offset dbufoff, 
					  int size,
					  void *dbufp,
					  void *paramp);
static int MPID_Segment_piece_unpack(DLOOP_Handle handle,
				     DLOOP_Offset dbufoff, 
				     int size,
				     void *dbufp,
				     void *paramp);

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
			    MPID_Segment_piece_pack, 
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

    MPID_Segment_manipulate(segp, first, lastp, 
			    MPID_Segment_piece_pack_vector, 
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
    MPID_Segment_manipulate(segp, first, lastp, 
			    MPID_Segment_piece_unpack, 
			    &unpack_params);
    return;
}


/* MPID_Segment_piece_pack_vector
 */
static int MPID_Segment_piece_pack_vector(DLOOP_Handle handle,
					  DLOOP_Offset dbufoff, 
					  int size,
					  void *dbufp,
					  void *v_paramp)
{
    /* TODO: IS THIS IN ANY WAY A BAD THING TO DO? */
    struct MPID_Segment_piece_params *paramp = v_paramp;
#ifdef MPID_SP_VERBOSE
    MPIU_dbg_printf("\t[index = %d, loc = (%x + %x) = %x, size = %d]\n",
		    paramp->u.pack_vector.index,
		    (unsigned) dbufp,
		    (unsigned) dbufoff,
		    (unsigned) dbufp + dbufoff,
		    size);
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

/* MPID_Segment_unpack_vector
 *
 * Q: Should this be any different from pack vector???
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

/* MPID_Segment_piece_unpack
 */
static int MPID_Segment_piece_unpack(DLOOP_Handle handle,
				     DLOOP_Offset dbufoff, 
				     int size,
				     void *dbufp,
				     void *v_paramp)
{
    struct MPID_Segment_piece_params *paramp = v_paramp;

#ifdef MPID_SU_VERBOSE
    dbg_printf("\t[h=%x, do=%d, dp=%x, bp=%x, sz=%d]\n", handle, dbufoff, 
	       (unsigned) dbufp, (unsigned) paramp->u.unpack.unpack_buffer, size);
#endif
    
    memcpy((char*)dbufp+dbufoff, paramp->u.unpack.unpack_buffer, size);
    paramp->u.unpack.unpack_buffer += size;
    return 0;
}

/* MPID_Segment_piece_pack
 */
static int MPID_Segment_piece_pack(DLOOP_Handle handle,
				   DLOOP_Offset dbufoff,
				   int size, 
				   void *dbufp,
				   void *v_paramp)
{
    struct MPID_Segment_piece_params *paramp = v_paramp;

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * bp = pack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[h=%x, do=%d, dp=%x, bp=%x, sz=%d]\n", handle, dbufoff, 
	       (unsigned) dbufp, (unsigned) paramp->u.pack.pack_buffer, size);
#endif

    memcpy(paramp->u.pack.pack_buffer, (char*)dbufp+dbufoff, size);
    paramp->u.pack.pack_buffer += size;
    return 0;
}

#if 0
/* MPID_Segment_piece_print
 */
static int MPID_Segment_piece_print(DLOOP_Handle handle,
				    DLOOP_Offset dbufoff,
				    int size,
				    void *dbufp,
				    void *v_paramp)
{
    struct MPID_Segment_piece_params *paramp = v_paramp;
#ifdef MPID_D_VERBOSE
    dbg_printf("\t[h=%x, do=%d, dp=%x, sz=%d]\n", handle, dbufoff, 
	       (unsigned) dbufp, size);
#endif
    return 0;
}
#endif
