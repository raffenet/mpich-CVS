/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPID_DATATYPE_H
#define MPID_DATATYPE_H

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

/* Datatype functions */
int MPID_Type_vector(int count,
		     int blocklength,
		     int stride,
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

struct MPID_Segment * MPID_Segment_alloc(void);

void MPID_Segment_free(struct MPID_Segment *segp);

int MPID_Segment_init(const void *buf,
		      int count,
		      MPI_Datatype handle,
		      struct MPID_Segment *segp);

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
#endif
