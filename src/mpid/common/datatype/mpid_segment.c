/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

int MPID_Segment_init(const void *buf, int count, MPI_Datatype handle, MPID_Segment *segp)
{
    return 0;
}

void MPID_Segment_pack(MPID_Segment *segp, int first, int *lastp, void *pack_buffer)
{
    return;
}

void MPID_Segment_pack_vector(MPID_Segment *segp, int first, int *lastp, MPID_VECTOR *vectorp, int *lengthp)
{
    return;
}

void MPID_Segment_unpack(MPID_Segment *segp, int first, int *lastp, void *unpack_buffer)
{
    return;
}

void MPID_Segment_unpack_vector(MPID_Segment *segp, int first, int *lastp, MPID_VECTOR *vectorp, int *lengthp)
{
    MPID_Segment_pack_vector(segp, first, lastp, vectorp, lengthp);
    return;
}
