/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIHANDLE_H
#define MPIHANDLE_H

/* Known MPI object types.  These are used for both the error handlers 
   and for the handles.  This is a 4 bit value.  0 is reserved for so 
   that all-zero handles can be flagged as an error. */
typedef enum MPID_Object_kind { 
  MPID_COMM       = 0x1, 
  MPID_GROUP      = 0x2,
  MPID_DATATYPE   = 0x3,
  MPID_FILE       = 0x4,
  MPID_ERRHANDLER = 0x5,
  MPID_OP         = 0x6,
  MPID_INFO       = 0x7,
  MPID_WIN        = 0x8,
  MPID_KEYVAL     = 0x9,
  MPID_ATTR       = 0xa,
  MPID_REQUEST    = 0xb
  } MPID_Object_kind;
/* The above objects should correspond to MPI objects only. */
#define HANDLE_MPI_KIND_SHIFT 26
#define HANDLE_GET_MPI_KIND(a) ( ((a)&0x3c000000) >> HANDLE_MPI_KIND_SHIFT )

/* Handle types.  These are really 2 bits */
#define HANDLE_KIND_INVALID  0x0
#define HANDLE_KIND_BUILTIN  0x1
#define HANDLE_KIND_DIRECT   0x2
#define HANDLE_KIND_INDIRECT 0x3
/* Mask assumes that ints are at least 4 bytes */
#define HANDLE_KIND_MASK 0xc0000000
#define HANDLE_KIND_SHIFT 30
#define HANDLE_GET_KIND(a) (((a)&HANDLE_KIND_MASK)>>HANDLE_KIND_SHIFT)
#define HANDLE_SET_KIND(a,kind) ((a)|((kind)<<HANDLE_KIND_SHIFT))

/* For indirect, the remainder of the handle has a block and index */
#define HANDLE_INDIRECT_SHIFT 16
#define HANDLE_BLOCK(a) (((a)& 0x03FF0000) >> HANDLE_INDIRECT_SHIFT)
#define HANDLE_BLOCK_INDEX(a) ((a) & 0x0000FFFF)

/* Handle block is between 1 and 1024 *elements* */
#define HANDLE_BLOCK_SIZE 256
/* Index size is bewtween 1 and 65536 *elements* */
#define HANDLE_BLOCK_INDEX_SIZE 1024

/* For direct, the remainder of the handle is the index into a predefined 
   block */
#define HANDLE_MASK 0x03FFFFFF
#define HANDLE_INDEX(a) ((a)& HANDLE_MASK)

/* ALL objects have the handle as the first value. */
/* Inactive (unused and stored on the appropriate avail list) objects 
   have MPIU_Handle_common as the head */
typedef struct MPIU_Handle_common {
    int  handle;
    volatile int ref_count; /* This field is used to indicate that the
			       object is not in use (see, e.g., 
			       MPID_Comm_valid_ptr) */
    void *next;   /* Free handles use this field to point to the next
                     free object */
} MPIU_Handle_common;

/* All *active* (in use) objects have the handle as the first value; objects
   with referene counts have the reference count as the second value.
   See MPIU_Object_add_ref and MPIU_Object_release_ref. */
typedef struct MPIU_Handle_head {
    int handle;
    volatile int ref_count;
} MPIU_Handle_head;

/* This type contains all of the data, except for the direct array,
   used by the object allocators. */
typedef struct MPIU_Object_alloc_t {
    MPIU_Handle_common *avail;          /* Next available object */
    int                initialized;     /* */
    void              *(*indirect)[];   /* Pointer to indirect object blocks */
    int                indirect_size;   /* Number of allocated indirect blocks */
    MPID_Object_kind   kind;            /* Kind of object this is for */
    int                size;            /* Size of an individual object */
    void               *direct;         /* Pointer to direct block, used 
                                           for allocation */
    int                direct_size;     /* Size of direct block */
} MPIU_Object_alloc_t;
extern void *MPIU_Handle_obj_alloc( MPIU_Object_alloc_t * );
extern void MPIU_Handle_obj_free( MPIU_Object_alloc_t *, void * );
void *MPIU_Handle_get_ptr_indirect( int, MPIU_Object_alloc_t * );

#endif
