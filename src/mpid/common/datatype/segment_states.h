/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SEGMENT_STATES_H
#define SEGMENT_STATES_H

#define MPID_STATE_LIST_SEGMENT \
MPID_STATE_MPID_SEGMENT_PACK, \
MPID_STATE_MPID_SEGMENT_PACK_VECTOR, \
MPID_STATE_MPID_SEGMENT_FLATTEN, \
MPID_STATE_MPID_SEGMENT_COUNT_CONTIG_BLOCKS, \
MPID_STATE_MPID_SEGMENT_UNPACK, \
MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_IOV, \
MPID_STATE_MPID_SEGMENT_VECTOR_FLATTEN, \
MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_IOV, \
MPID_STATE_MPID_SEGMENT_CONTIG_FLATTEN, \
MPID_STATE_MPID_SEGMENT_CONTIG_COUNT_BLOCK, \
MPID_STATE_MPID_SEGMENT_UNPACK_VECTOR, \
MPID_STATE_MPID_SEGMENT_VECTOR_UNPACK_TO_BUF, \
MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_TO_BUF, \
MPID_STATE_MPID_SEGMENT_VECTOR_PACK_TO_BUF, \
MPID_STATE_MPID_SEGMENT_CONTIG_PACK_TO_BUF,

#endif
