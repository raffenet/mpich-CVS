/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpid_datatype.h"

int MPID_Segment_init(void *buf, int count, MPI_Datatype handle, MPID_Segment *segp)
{
    segp->bogus_dtype_extent = MPID_Datatype_get_size(handle);
    segp->bogus_user_buffer = buf;
    segp->bogus_length = count * segp->bogus_dtype_extent;
    return 0;
}

void MPID_Segment_pack(MPID_Segment *segp, int first, int *lastp, void *pack_buffer)
{
    memcpy(pack_buffer, segp->bogus_user_buffer, *lastp - first);
    return;
}

void MPID_Segment_pack_vector(MPID_Segment *segp, int first, int *lastp, MPID_VECTOR *vectorp, int *lengthp)
{
    vectorp[0].MPID_VECTOR_BUF = (char*)segp->bogus_user_buffer + (first * segp->bogus_dtype_extent);
    vectorp[0].MPID_VECTOR_LEN = *lastp - first;
    *lengthp = 1;
    return;
}

void MPID_Segment_unpack(MPID_Segment *segp, int first, int *lastp, const void *unpack_buffer)
{
    memcpy(segp->bogus_user_buffer, unpack_buffer, *lastp - first);
    return;
}

void MPID_Segment_unpack_vector(MPID_Segment *segp, int first, int *lastp, MPID_VECTOR *vectorp, int *lengthp)
{
    MPID_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);
    return;
}
