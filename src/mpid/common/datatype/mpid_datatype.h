/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPID_DATATYPE_H
#define MPID_DATATYPE_H

#include "mpiimpl.h"
#include "mpid_dataloop.h"
#include "mpihandlemem.h"

/* NOTE: 
 * - struct MPID_Dataloop and MPID_Segment are defined in 
 *   src/mpid/common/datatype/mpid_dataloop.h (and gen_dataloop.h).
 * - MPIU_Object_alloc_t is defined in src/include/mpihandle.h
 */

#define MPID_Datatype_get_ptr(a,ptr)   MPID_Getb_ptr(Datatype,a,0x000000ff,ptr)
#define MPID_Datatype_get_basic_size(a) (((a)&0x0000ff00)>>8)

#define MPID_Datatype_add_ref(datatype_ptr) MPIU_Object_add_ref((datatype_ptr))

#define MPID_Datatype_get_basic_type(a,__eltype)			\
do {									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            __eltype = ((MPID_Datatype *) ptr)->eltype;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            __eltype = ((MPID_Datatype *) ptr)->eltype;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            __eltype = a;						\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    __eltype = 0;						\
	    break;							\
 									\
    }									\
} while (0)

/* MPID_Datatype_release decrements the reference count on the MPID_Datatype
 * and, if the refct is then zero, frees the MPID_Datatype and associated
 * structures.
 */
#define MPID_Datatype_release(datatype_ptr)					\
do {										\
    int inuse;									\
										\
    MPIU_Object_release_ref((datatype_ptr),&inuse);				\
    if (!inuse) {								\
        int lmpi_errno = MPI_SUCCESS;						\
	if (MPIR_Process.attr_free && datatype_ptr->attributes) {		\
	    lmpi_errno = MPIR_Process.attr_free( datatype_ptr->handle,		\
						datatype_ptr->attributes );	\
	}									\
 	/* LEAVE THIS COMMENTED OUT UNTIL WE HAVE SOME USE FOR THE FREE_FN	\
	if (datatype_ptr->free_fn) {						\
	    mpi_errno = (datatype_ptr->free_fn)( datatype_ptr );		\
	     if (mpi_errno) {							\
		 MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_TYPE_FREE);			\
		 return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );		\
	     }									\
	} */									\
        if (lmpi_errno == MPI_SUCCESS) {					\
	    MPID_Datatype_free(datatype_ptr);					\
        }									\
    }										\
} while (0)

/* Note: Probably there is some clever way to build all of these from a macro.
 */
#define MPID_Datatype_get_size_macro(a,__size)				\
do {									\
    void *ptr;								\
    switch (HANDLE_GET_KIND(a)) {					\
        case HANDLE_KIND_DIRECT:					\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);			\
            __size = ((MPID_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_INDIRECT:					\
            ptr = ((MPID_Datatype *)					\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));	\
            __size = ((MPID_Datatype *) ptr)->size;			\
            break;							\
        case HANDLE_KIND_BUILTIN:					\
            __size = MPID_Datatype_get_basic_size(a);			\
            break;							\
        case HANDLE_KIND_INVALID:					\
        default:							\
	    __size = 0;							\
	    break;							\
 									\
    }									\
} while (0)

#define MPID_Datatype_get_loopdepth_macro(a,__depth)                    \
do {                                                                    \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);                 \
            __depth = ((MPID_Datatype *) ptr)->loopinfo_depth;          \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPID_Datatype *)                                    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem)); \
            __depth = ((MPID_Datatype *) ptr)->loopinfo_depth;          \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            __depth = 0;                                                \
            break;                                                      \
    }                                                                   \
} while (0)

#define MPID_Datatype_get_loopptr_macro(a,__lptr)                       \
do {                                                                    \
    void *ptr;                                                          \
    switch (HANDLE_GET_KIND(a)) {                                       \
        case HANDLE_KIND_DIRECT:                                        \
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);                 \
            __lptr = ((MPID_Datatype *) ptr)->loopinfo;                 \
            break;                                                      \
        case HANDLE_KIND_INDIRECT:                                      \
            ptr = ((MPID_Datatype *)                                    \
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem)); \
            __lptr = ((MPID_Datatype *) ptr)->loopinfo;                 \
            break;                                                      \
        case HANDLE_KIND_INVALID:                                       \
        case HANDLE_KIND_BUILTIN:                                       \
        default:                                                        \
            __lptr = 0;                                                 \
            break;                                                      \
    }                                                                   \
} while (0)
        
#define MPID_Datatype_get_extent_macro(a,__extent)				\
do {										\
    void *ptr;									\
    switch (HANDLE_GET_KIND(a)) {						\
        case HANDLE_KIND_DIRECT:						\
            ptr = MPID_Datatype_direct+HANDLE_INDEX(a);				\
            __extent = ((MPID_Datatype *) ptr)->extent;				\
            break;								\
        case HANDLE_KIND_INDIRECT:						\
            ptr = ((MPID_Datatype *)						\
		   MPIU_Handle_get_ptr_indirect(a,&MPID_Datatype_mem));		\
            __extent = ((MPID_Datatype *) ptr)->extent;				\
            break;								\
        case HANDLE_KIND_INVALID:						\
        case HANDLE_KIND_BUILTIN:						\
        default:								\
            __extent = MPID_Datatype_get_basic_size(a);  /* same as size */	\
            break;								\
    }										\
} while (0)

#define MPID_Datatype_valid_ptr(ptr,err) MPID_Valid_ptr_class(Datatype,ptr,MPI_ERR_TYPE,err)

/* to be used only after MPID_Datatype_valid_ptr(); the check on
 * err == MPI_SUCCESS ensures that we won't try to dereference the
 * pointer if something has already been detected as wrong.
 */
#define MPID_Datatype_committed_ptr(ptr,err)				\
do {									\
    if ((err == MPI_SUCCESS) && !((ptr)->is_committed))			\
        err = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE, "**dtypecommit", 0);	\
} while (0)

typedef struct MPID_Datatype_contents {
    int combiner;
    int nr_ints;
    int nr_aints;
    int nr_types;
    /* space allocated beyond structure used to store the types[], ints[], aints[], in that order */
} MPID_Datatype_contents;

/* Datatype Structure */
/*S
  MPID_Datatype - Description of the MPID Datatype structure

  Notes:
  The 'ref_count' is needed for nonblocking operations such as
.vb
   MPI_Type_struct( ... , &newtype );
   MPI_Irecv( buf, 1000, newtype, ..., &request );
   MPI_Type_free( &newtype );
   ...
   MPI_Wait( &request, &status );
.ve

  Module:
  Datatype-DS

  Notes:

  Alternatives:
  The following alternatives for the layout of this structure were considered.
  Most were not chosen because any benefit in performance or memory 
  efficiency was outweighed by the added complexity of the implementation.

  A number of fields contain only boolean inforation ('is_contig', 
  'has_sticky_ub', 'has_sticky_lb', 'is_permanent', 'is_committed').  These 
  could be combined and stored in a single bit vector.  

  'MPI_Type_dup' could be implemented with a shallow copy, where most of the
  data fields, particularly the 'opt_dataloop' field, would not be copied into
  the new object created by 'MPI_Type_dup'; instead, the new object could 
  point to the data fields in the old object.  However, this requires 
  more code to make sure that fields are found in the correct objects and that
  deleting the old object doesn't invalidate the dup'ed datatype.

  A related optimization would point to the 'opt_dataloop' and 'dataloop' 
  fields in other datatypes.  This has the same problems as the shallow 
  copy implementation.

  In addition to the separate 'dataloop' and 'opt_dataloop' fields, we could
  in addition have a separate 'hetero_dataloop' optimized for heterogeneous
  communication for systems with different data representations. 

  Earlier versions of the ADI used a single API to change the 'ref_count', 
  with each MPI object type having a separate routine.  Since reference
  count changes are always up or down one, and since all MPI objects 
  are defined to have the 'ref_count' field in the same place, the current
  ADI3 API uses two routines, 'MPIU_Object_add_ref' and 
  'MPIU_Object_release_ref', to increment and decrement the reference count.

  S*/
typedef struct MPID_Datatype { 
    int           handle;            /* value of MPI_Datatype for structure */
    volatile int  ref_count;
    int           is_contig;     /* True if data is contiguous (even with 
                                    a (count,datatype) pair) */
    int           n_contig_blocks; /* number of contiguous blocks in one instance of this type */
    int           size;          /* Q: maybe this should be in the dataloop? */
    MPI_Aint      extent;        /* MPI-2 allows a type to be created by
                                    resizing (the extent of) an existing 
                                    type */
    MPI_Aint      ub, lb,        /* MPI-1 upper and lower bounds */
                  true_ub, true_lb; /* MPI-2 true upper and lower bounds */
    int           alignsize;     /* size of datatype to align (affects pad) */
    /* The remaining fields are required but less frequently used, and
       are placed after the more commonly used fields */
    int loopsize; /* size of loops for this datatype in bytes; derived value */
    struct MPID_Dataloop *loopinfo; /* Original loopinfo, used when 
                                     * creating and when getting contents */
    int           has_sticky_ub;   /* The MPI_UB and MPI_LB are sticky */
    int           has_sticky_lb;
    int           is_permanent;  /* True if datatype is a predefined type */
    int           is_committed;  /* */

    int           eltype;      /* type of elementary datatype. Needed
                                 to implement MPI_Accumulate */

    int           loopinfo_depth; /* Depth of dataloop stack needed
                                     to process this datatype.  This 
                                     information is used to ensure that
                                     no datatype is constructed that
                                     cannot be processed (see MPID_Segment) */

    struct MPID_Attribute   *attributes;    /* MPI-2 adds datatype attributes */

    int32_t       cache_id;      /* These are used to track which processes */
    /* MPID_Lpidmask mask; */         /* have cached values of this datatype */

    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */

    /* The following is needed to efficiently implement MPI_Get_elements */
    int           n_elements;   /* Number of basic elements in this datatype */
    MPI_Aint      element_size; /* Size of each element or -1 if elements are
                                   not all the same size */
    MPID_Datatype_contents *contents;
    /* int (*free_fn)( struct MPID_Datatype * ); */ /* Function to free this datatype */
    /* Other, device-specific information */
#ifdef MPID_DEV_DATATYPE_DECL
    MPID_DEV_DATATYPE_DECL
#endif
} MPID_Datatype;

extern MPIU_Object_alloc_t MPID_Datatype_mem;

/* Preallocated datatype objects */
#define MPID_DATATYPE_N_BUILTIN 35
extern MPID_Datatype MPID_Datatype_builtin[MPID_DATATYPE_N_BUILTIN + 1];
extern MPID_Datatype MPID_Datatype_direct[];

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
int MPID_Type_commit(MPI_Datatype *type);

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

int MPID_Type_create_resized(MPI_Datatype oldtype,
			     MPI_Aint lb,
			     MPI_Aint extent,
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
void MPID_Dataloop_copy(void *dest,
			void *src,
			int size);

void MPID_Dataloop_print(struct MPID_Dataloop *dataloop,
			 int depth);

struct MPID_Dataloop * MPID_Dataloop_alloc(void);

void MPID_Dataloop_free(struct MPID_Dataloop *dataloop);

/* Segment functions */
void MPID_Segment_pack(struct DLOOP_Segment *segp,
		       DLOOP_Offset first,
		       DLOOP_Offset *lastp,
		       void *pack_buffer);

void MPID_Segment_unpack(struct DLOOP_Segment *segp,
			 DLOOP_Offset first,
			 DLOOP_Offset *lastp,
			 const void * unpack_buffer);

void MPID_Segment_pack_vector(struct DLOOP_Segment *segp,
			      DLOOP_Offset first,
			      DLOOP_Offset *lastp,
			      DLOOP_VECTOR *vector,
			      int *lengthp);

void MPID_Segment_unpack_vector(struct DLOOP_Segment *segp,
				DLOOP_Offset first,
				DLOOP_Offset *lastp,
				DLOOP_VECTOR *vector,
				int *lengthp);

void MPID_Segment_count_contig_blocks(struct DLOOP_Segment *segp,
				      DLOOP_Offset first,
				      DLOOP_Offset *lastp,
				      DLOOP_Offset *countp);

void MPID_Segment_flatten(struct DLOOP_Segment *segp,
			  DLOOP_Offset first,
			  DLOOP_Offset *lastp,
			  DLOOP_Offset *offp,
			  int *sizep,
			  DLOOP_Offset *lengthp);

/* misc */
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

void DLOOP_Dataloop_update(struct DLOOP_Dataloop *dataloop,
                           MPI_Aint ptrdiff);

int MPIR_Type_get_contig_blocks(MPI_Datatype type,
				int *nr_blocks_p);

int MPIR_Type_flatten(MPI_Datatype type,
		      MPI_Aint *off_array,
		      int *size_array,
		      MPI_Aint *array_len_p);
/* end of file */
#endif
