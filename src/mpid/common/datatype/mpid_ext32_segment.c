/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpichconf.h>
#include <mpiimpl.h>
#include <mpid_dataloop.h>

#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

#include "mpid_ext32_segment.h"

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
	    int64_t *offp;
	    int *sizep; /* see notes in Segment_flatten header */
            int index;
            int length;
        } flatten;
	struct {
	    char *last_loc;
	    int count;
	} contig_blocks;
        struct {
            char *unpack_buffer;
        } unpack;
        struct {
            int stream_off;
        } print;
    } u;
};

typedef struct
{
    DLOOP_Type el_type;
    DLOOP_Offset el_size;
} external32_basic_size_t;

static external32_basic_size_t external32_basic_size_array[] =
{
    { MPI_PACKED, 1 },
    { MPI_BYTE, 1 },
    { MPI_CHAR, 1 },
    { MPI_UNSIGNED_CHAR, 1 },
/*     { MPI_SIGNED_CHAR, 1 }, */
/*     { MPI_WCHAR, 2 }, */
    { MPI_SHORT, 2 },
    { MPI_UNSIGNED_SHORT, 2 },
    { MPI_INT, 4 },
    { MPI_UNSIGNED, 4 },
    { MPI_LONG, 4 },
    { MPI_UNSIGNED_LONG, 4 },
    { MPI_FLOAT, 4 },
    { MPI_DOUBLE, 8 },
    { MPI_LONG_DOUBLE, 16 },
    { MPI_CHARACTER, 1 },
    { MPI_LOGICAL, 4 },
    { MPI_INTEGER, 4 },
    { MPI_REAL, 4 }, 
    { MPI_DOUBLE_PRECISION, 8 },
    { MPI_COMPLEX, 8 },
    { MPI_DOUBLE_COMPLEX, 16 },
/*     { MPI_INTEGER1, 1 }, */
/*     { MPI_INTEGER2, 2 }, */
/*     { MPI_INTEGER4, 4 }, */
/*     { MPI_INTEGER8, 8 }, */
    { MPI_LONG_LONG, 8 },
/*     { MPI_UNSIGNED_LONG_LONG, 8 } */
/*     { MPI_REAL4, 4 }, */
/*     { MPI_REAL8, 8 }, */
/*     { MPI_REAL16, 16 } */
};

static DLOOP_Offset MPID_Segment_get_external32_basic_size(DLOOP_Type el_type)
{
    DLOOP_Offset ret = (DLOOP_Offset)0;
    int i = 0;
    for(i = 0; i < (sizeof(external32_basic_size_array) /
                    sizeof(external32_basic_size_t)); i++)
    {
        if (external32_basic_size_array[i].el_type == el_type)
        {
            ret = external32_basic_size_array[i].el_size;
            break;
        }
    }
    return ret;
}

static inline is_float_type(DLOOP_Type el_type)
{
    return ((el_type == MPI_FLOAT) || (el_type == MPI_DOUBLE) ||
            (el_type == MPI_LONG_DOUBLE) ||
            (el_type == MPI_DOUBLE_PRECISION) ||
            (el_type == MPI_COMPLEX) || (el_type == MPI_DOUBLE_COMPLEX));
/*             (el_type == MPI_REAL4) || (el_type == MPI_REAL8) || */
/*             (el_type == MPI_REAL16)); */
}

int external32_basic_convert(char *dest_buf,
                             char *src_buf,
                             int dest_el_size,
                             int src_el_size,
                             DLOOP_Offset count)
{
    char *src_ptr = src_buf, *dest_ptr = dest_buf;
    char *src_end = (char *)(src_buf + ((int)count * src_el_size));
    char *dest_end = (char *)(dest_buf + ((int)count * dest_el_size));

    assert(dest_buf && src_buf);

    if ((src_el_size == dest_el_size) && (src_el_size == 2))
    {
        while(src_ptr != src_end)
        {
            BASIC_convert16((*(TWO_BYTE_BASIC_TYPE *)src_ptr),
                            (*(TWO_BYTE_BASIC_TYPE *)dest_ptr));

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else if ((src_el_size == dest_el_size) && (src_el_size == 4))
    {
        while(src_ptr != src_end)
        {
            BASIC_convert32((*(FOUR_BYTE_BASIC_TYPE *)src_ptr),
                            (*(FOUR_BYTE_BASIC_TYPE *)dest_ptr));

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else if ((src_el_size == dest_el_size) && (src_el_size == 8))
    {
        while(src_ptr != src_end)
        {
            BASIC_convert64((EIGHT_BYTE_BASIC_TYPE *)src_ptr,
                            (EIGHT_BYTE_BASIC_TYPE *)dest_ptr);

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else if ((src_el_size == dest_el_size) && (src_el_size == 12))
    {
        while(src_ptr != src_end)
        {
            BASIC_convert96(src_ptr, dest_ptr);

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else
    {
        /* TODO */
    }
    return 0;
}

int external32_float_convert(char *dest_buf,
                             char *src_buf,
                             int dest_el_size,
                             int src_el_size,
                             int count)
{
    char *src_ptr = src_buf, *dest_ptr = dest_buf;
    char *src_end = (char *)(src_buf + ((int)count * src_el_size));
    char *dest_end = (char *)(dest_buf + ((int)count * dest_el_size));

    assert(dest_buf && src_buf);

    if ((src_el_size == dest_el_size) && (src_el_size == 4))
    {
        while(src_ptr != src_end)
        {
            FLOAT_convert((*(FOUR_BYTE_FLOAT_TYPE *)src_ptr),
                          (*(FOUR_BYTE_FLOAT_TYPE *)dest_ptr));

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else if ((src_el_size == dest_el_size) && (src_el_size == 8))
    {
        while(src_ptr != src_end)
        {
            FLOAT_convert((*(EIGHT_BYTE_FLOAT_TYPE *)src_ptr),
                          (*(EIGHT_BYTE_FLOAT_TYPE *)dest_ptr));

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else if ((src_el_size == dest_el_size) && (src_el_size == 12))
    {
        while(src_ptr != src_end)
        {
            FLOAT_convert((*(TWELVE_BYTE_FLOAT_TYPE *)src_ptr),
                          (*(TWELVE_BYTE_FLOAT_TYPE *)dest_ptr));

            src_ptr += src_el_size;
            dest_ptr += dest_el_size;
        }
    }
    else
    {
        /* TODO */
    }
    return 0;
}

static int MPID_Segment_contig_pack_external32_to_buf(DLOOP_Offset *blocks_p,
                                                      DLOOP_Type el_type,
                                                      DLOOP_Offset rel_off,
                                                      void *bufp,
                                                      void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);

    src_el_size = MPID_Datatype_get_basic_size(el_type);
    dest_el_size = MPID_Segment_get_external32_basic_size(el_type);
    assert(dest_el_size);

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * bp = pack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[contig pack [external32]: do=%d, dp=%x, bp=%x, "
               "src_el_sz=%d, dest_el_sz=%d, blksz=%d]\n",
	       rel_off, 
	       (unsigned) bufp,
	       (unsigned) paramp->u.pack.pack_buffer,
	       src_el_size,
	       dest_el_size,
	       (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */
    if ((src_el_size == dest_el_size) && (src_el_size == 1))
    {
        memcpy(paramp->u.pack.pack_buffer, (char *)(bufp + rel_off), *blocks_p);
    }
    else if (is_float_type(el_type))
    {
        external32_float_convert(paramp->u.pack.pack_buffer, (char *)(bufp + rel_off),
                                 dest_el_size, src_el_size, *blocks_p);
    }
    else
    {
        external32_basic_convert(paramp->u.pack.pack_buffer, (char *)(bufp + rel_off),
                                 dest_el_size, src_el_size, *blocks_p);
    }
    paramp->u.pack.pack_buffer += (dest_el_size * (*blocks_p));

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_PACK_EXTERNAL32_TO_BUF);
    return 0;
}

static int MPID_Segment_contig_unpack_external32_to_buf(DLOOP_Offset *blocks_p,
                                                        DLOOP_Type el_type,
                                                        DLOOP_Offset rel_off,
                                                        void *bufp,
                                                        void *v_paramp)
{
    int src_el_size, dest_el_size;
    struct MPID_Segment_piece_params *paramp = v_paramp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);

    src_el_size = MPID_Datatype_get_basic_size(el_type);
    dest_el_size = MPID_Segment_get_external32_basic_size(el_type);
    assert(dest_el_size);

    /*
     * h  = handle value
     * do = datatype buffer offset
     * dp = datatype buffer pointer
     * up = unpack buffer pointer (current location, incremented as we go)
     * sz = size of datatype (guess we could get this from handle value if
     *      we wanted...)
     */
#ifdef MPID_SP_VERBOSE
    dbg_printf("\t[contig unpack [external32]: do=%d, dp=%x, up=%x, "
               "src_el_sz=%d, dest_el_sz=%d, blksz=%d]\n",
	       rel_off,
	       (unsigned) bufp,
	       (unsigned) paramp->u.unpack.unpack_buffer,
	       src_el_size,
	       dest_el_size,
	       (int) *blocks_p);
#endif

    /* TODO: DEAL WITH CASE WHERE ALL DATA DOESN'T FIT! */
    if ((src_el_size == dest_el_size) && (src_el_size == 1))
    {
        memcpy((char *)(bufp + rel_off), paramp->u.unpack.unpack_buffer, *blocks_p);
    }
    else if (is_float_type(el_type))
    {
        external32_float_convert((char *)(bufp + rel_off), paramp->u.unpack.unpack_buffer,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    else
    {
        external32_basic_convert((char *)(bufp + rel_off), paramp->u.unpack.unpack_buffer,
                                 dest_el_size, src_el_size, *blocks_p);
    }
    paramp->u.unpack.unpack_buffer += (dest_el_size * (*blocks_p));

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF);
    return 0;
}

void MPID_Segment_pack_external(struct DLOOP_Segment *segp,
                                DLOOP_Offset first,
                                DLOOP_Offset *lastp, 
                                void *pack_buffer)
{
    struct MPID_Segment_piece_params pack_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_PACK_EXTERNAL);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_PACK_EXTERNAL);

    pack_params.u.pack.pack_buffer = (DLOOP_Buffer)pack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_pack_external32_to_buf,
                            NULL, /* MPID_Segment_vector_pack_external32_to_buf, */
                            NULL, /* MPID_Segment_index_pack_external32_to_buf, */
                            MPID_Segment_get_external32_basic_size,
			    &pack_params);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_PACK_EXTERNAL);
    return;
}

void MPID_Segment_unpack_external(struct DLOOP_Segment *segp,
                                  DLOOP_Offset first,
                                  DLOOP_Offset *lastp,
                                  DLOOP_Buffer unpack_buffer)
{
    struct MPID_Segment_piece_params pack_params;
    MPIDI_STATE_DECL(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL);

    pack_params.u.unpack.unpack_buffer = unpack_buffer;
    MPID_Segment_manipulate(segp,
			    first,
			    lastp,
			    MPID_Segment_contig_unpack_external32_to_buf,
                            NULL, /* MPID_Segment_vector_unpack_external32_to_buf, */
                            NULL, /* MPID_Segment_index_unpack_external32_to_buf, */
                            MPID_Segment_get_external32_basic_size,
			    &pack_params);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL);
    return;
}
