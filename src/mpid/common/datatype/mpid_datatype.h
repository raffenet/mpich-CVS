/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPID_DATATYPE_H
#define MPID_DATATYPE_H

#include <mpid_dataloop.h>

/* NOTE: 
 * - struct MPID_Datatype is defined in src/include/mpiimpl.h.
 * - struct MPID_Dataloop and MPID_Segment are defined in 
 *   src/mpid/common/datatype/mpid_dataloop.h (and gen_dataloop.h).
 */

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#define MPID_IOV         WSABUF
#define MPID_IOV_LEN     len
#define MPID_IOV_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_IOV         struct iovec
#define MPID_IOV_LEN     iov_len
#define MPID_IOV_BUF     iov_base
#endif
#define MPID_IOV_LIMIT   16

#define MPID_DTYPE_BEGINNING  0
#define MPID_DTYPE_END       -1

/* LB/UB calculation helper macros */

#define MPID_DATATYPE_CONTIG_LB_UB(__cnt, __old_lb, __old_ub, __old_extent, __lb, __ub)	\
do {											\
    if (__cnt == 0) {									\
	__lb = __old_lb;								\
	__ub = __old_ub;								\
    }											\
    else if (__old_ub >= __old_lb) {							\
        __lb = __old_lb;								\
        __ub = __old_ub + (__old_extent) * (__cnt - 1);					\
    }											\
    else /* negative extent */ {							\
	__lb = __old_lb + (__old_extent) * (__cnt - 1);					\
	__ub = __old_ub;								\
    }											\
} while (0)

/* MPID_DATATYPE_VECTOR_LB_UB()
 *
 */
#define MPID_DATATYPE_VECTOR_LB_UB(__cnt, __stride, __blklen, __old_lb, __old_ub, __old_extent, __lb, __ub) \
do {													    \
    if (__cnt == 0 || __blklen == 0) {									    \
	__lb = __old_lb;										    \
	__ub = __old_ub;										    \
    }													    \
    else if (__stride >= 0 && (__old_extent) >= 0) {							    \
	__lb = __old_lb;										    \
	__ub = __old_ub + (__old_extent) * ((__blklen) - 1) + (__stride) * ((__cnt) - 1);		    \
    }													    \
    else if (__stride < 0 && (__old_extent) >= 0) {							    \
	__lb = __old_lb + (__stride) * ((__cnt) - 1);							    \
	__ub = __old_ub + (__old_extent) * ((__blklen) - 1);						    \
    }													    \
    else if (__stride >= 0 && (__old_extent) < 0) {							    \
	__lb = __old_lb + (__old_extent) * ((__blklen) - 1);						    \
	__ub = __old_ub + (__stride) * ((__cnt) - 1);							    \
    }													    \
    else {												    \
	__lb = __old_lb + (__old_extent) * ((__blklen) - 1) + (__stride) * ((__cnt) - 1);		    \
	__ub = __old_ub;										    \
    }													    \
} while (0)

/* MPID_DATATYPE_BLOCK_LB_UB()
 *
 * Note: we need the extent here in addition to the lb and ub because the
 * extent might have some padding in it that we need to take into account.
 */
#define MPID_DATATYPE_BLOCK_LB_UB(__cnt, __disp, __old_lb, __old_ub, __old_extent, __lb, __ub)	\
do {												\
    if (__cnt == 0) {										\
	__lb = __old_lb + (__disp);								\
	__ub = __old_ub + (__disp);								\
    }												\
    else if (__old_ub >= __old_lb) {								\
        __lb = __old_lb + (__disp);								\
        __ub = __old_ub + (__disp) + (__old_extent) * ((__cnt) - 1);				\
    }												\
    else /* negative extent */ {								\
	__lb = __old_lb + (__disp) + (__old_extent) * ((__cnt) - 1);				\
	__ub = __old_ub + (__disp);								\
    }												\
} while (0)

/* Datatype functions */
int MPID_Type_dup(MPI_Datatype oldtype,
		  MPI_Datatype *newtype);

int MPID_Type_struct(int count,
		     int *blocklength_array,
		     MPI_Aint *displacement_array,
		     MPI_Datatype *oldtype_array,
		     MPI_Datatype *newtype);

int MPID_Type_indexed(int count,
		      int *blocklength_array,
		      void *displacement_array,
		      int dispinbytes,
		      MPI_Datatype oldtype,
		      MPI_Datatype *newtype);

int MPID_Type_vector(int count,
		     int blocklength,
		     MPI_Aint stride,
		     int strideinbytes,
		     MPI_Datatype oldtype,
		     MPI_Datatype *newtype);

int MPID_Type_contiguous(int count,
			 MPI_Datatype oldtype,
			 MPI_Datatype *newtype);

int MPID_Type_get_envelope(MPI_Datatype datatype,
			   int *num_integers,
			   int *num_addresses,
			   int *num_datatypes,
			   int *combiner);

int MPID_Type_get_contents(MPI_Datatype datatype, 
			   int max_integers, 
			   int max_addresses, 
			   int max_datatypes, 
			   int array_of_integers[], 
			   MPI_Aint array_of_addresses[], 
			   MPI_Datatype array_of_datatypes[]);

/* internal debugging functions */
void MPIDI_Datatype_printf(MPI_Datatype type,
			   int depth,
			   MPI_Aint displacement,
			   int blocklength,
			   int header);

/* Dataloop functions */

typedef struct MPID_Dataloop * MPID_Dataloop_foo; /* HACK */

void MPID_Dataloop_copy(void *dest,
			void *src,
			int size);

void MPID_Dataloop_print(struct MPID_Dataloop *dataloop,
			 int depth);

struct MPID_Dataloop * MPID_Dataloop_alloc(void);

void MPID_Dataloop_free(struct MPID_Dataloop *dataloop);

/* Segment functions */

typedef struct MPID_Segment * MPID_Segment_foo; /* HACK */

void MPID_Segment_pack(struct MPID_Segment *segp,
		       int first,
		       int *lastp,
		       void *pack_buffer);

void MPID_Segment_unpack(struct MPID_Segment *segp,
			 int first,
			 int *lastp,
			 const void * unpack_buffer);

void MPID_Segment_pack_vector(struct MPID_Segment *segp,
			      int first,
			      int *lastp,
			      MPID_IOV *vector,
			      int *lengthp);

void MPID_Segment_unpack_vector(struct MPID_Segment *segp,
				int first,
				int *lastp,
				MPID_IOV *vector,
				int *lengthp);

/* misc */


typedef struct MPID_Datatype * MPID_Datatype_foo; /* HACK */

int MPID_Datatype_set_contents(struct MPID_Datatype *ptr,
			       int combiner,
			       int nr_ints,
			       int nr_aints,
			       int nr_types,
			       int *ints,
			       MPI_Aint *aints,
			       MPI_Datatype *types);

void MPID_Datatype_free_contents(struct MPID_Datatype *ptr);

void MPID_Datatype_free(struct MPID_Datatype *ptr);


/* end of file */
#endif
