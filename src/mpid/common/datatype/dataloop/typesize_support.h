/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef TYPESIZE_SUPPORT_H
#define TYPESIZE_SUPPORT_H

#include <mpid_dataloop.h>

#define DLOOP_Type_footprint        PREPEND_PREFIX(Type_footprint)

typedef struct PREPEND_PREFIX(Type_footprint_s) {
    DLOOP_Offset size;
    DLOOP_Offset extent;

    /* these are only needed for calculating footprint of types
     * built using this type. no reason to expose these.
     */
    DLOOP_Offset lb;
    DLOOP_Offset ub;
    DLOOP_Offset alignsz;
    int has_sticky_lb;
    int has_sticky_ub;
} DLOOP_Type_footprint;

void PREPEND_PREFIX(Type_calc_footprint)(MPI_Datatype type,
					 DLOOP_Type_footprint *tfp);

/* LB/UB calculation helper macros, from MPICH2 */

/* DLOOP_DATATYPE_CONTIG_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the
 * old type's LB, UB, and extent, and a count of these types in the
 * block.
 *
 * Note: if the displacement is non-zero, the DLOOP_DATATYPE_BLOCK_LB_UB()
 * should be used instead (see below).
 */
#define DLOOP_DATATYPE_CONTIG_LB_UB(cnt_,			\
				    old_lb_,			\
				    old_ub_,			\
				    old_extent_,		\
				    lb_,			\
				    ub_)			\
    {								\
	if (cnt_ == 0) {					\
	    lb_ = old_lb_;					\
	    ub_ = old_ub_;					\
	}							\
	else if (old_ub_ >= old_lb_) {				\
	    lb_ = old_lb_;					\
	    ub_ = old_ub_ + (old_extent_) * (cnt_ - 1);		\
	}							\
	else /* negative extent */ {				\
	    lb_ = old_lb_ + (old_extent_) * (cnt_ - 1);		\
	    ub_ = old_ub_;					\
	}							\
    }

/* DLOOP_DATATYPE_VECTOR_LB_UB()
 *
 * Determines the new LB and UB for a vector of blocks of old types
 * given the old type's LB, UB, and extent, and a count, stride, and
 * blocklen describing the vectorization.
 */
#define DLOOP_DATATYPE_VECTOR_LB_UB(cnt_,			\
				    stride_,			\
				    blklen_,			\
				    old_lb_,			\
				    old_ub_,			\
				    old_extent_,		\
				    lb_,			\
				    ub_)			\
    {								\
	if (cnt_ == 0 || blklen_ == 0) {			\
	    lb_ = old_lb_;					\
	    ub_ = old_ub_;					\
	}							\
	else if (stride_ >= 0 && (old_extent_) >= 0) {		\
	    lb_ = old_lb_;					\
	    ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1) +	\
		(stride_) * ((cnt_) - 1);			\
	}							\
	else if (stride_ < 0 && (old_extent_) >= 0) {		\
	    lb_ = old_lb_ + (stride_) * ((cnt_) - 1);		\
	    ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1);	\
	}							\
	else if (stride_ >= 0 && (old_extent_) < 0) {		\
	    lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1);	\
	    ub_ = old_ub_ + (stride_) * ((cnt_) - 1);		\
	}							\
	else {							\
	    lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1) +	\
		(stride_) * ((cnt_) - 1);			\
	    ub_ = old_ub_;					\
	}							\
    }

/* DLOOP_DATATYPE_BLOCK_LB_UB()
 *
 * Determines the new LB and UB for a block of old types given the LB,
 * UB, and extent of the old type as well as a new displacement and count
 * of types.
 *
 * Note: we need the extent here in addition to the lb and ub because the
 * extent might have some padding in it that we need to take into account.
 */
#define DLOOP_DATATYPE_BLOCK_LB_UB(cnt_,				\
				   disp_,				\
				   old_lb_,				\
				   old_ub_,				\
				   old_extent_,				\
				   lb_,					\
				   ub_)					\
    {									\
	if (cnt_ == 0) {						\
	    lb_ = old_lb_ + (disp_);					\
	    ub_ = old_ub_ + (disp_);					\
	}								\
	else if (old_ub_ >= old_lb_) {					\
	    lb_ = old_lb_ + (disp_);					\
	    ub_ = old_ub_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
	}								\
	else /* negative extent */ {					\
	    lb_ = old_lb_ + (disp_) + (old_extent_) * ((cnt_) - 1);	\
	    ub_ = old_ub_ + (disp_);					\
	}								\
    }

#endif
