/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPID_DATATYPE_H
#define MPID_DATATYPE_H

/*
 * Each of the MPI datatypes can be mapped into one of 5 very simple
 * loops.  This loop has the following parameters:
 *    count  blocksize[] offset[]/stride datatype[]
 * where each [] indicates that a field may be *either* an array or a scalar.
 * For each such type, we define a struct that describes these parameters
 */

/*S
  MPID_Dataloop_contig - Description of a contiguous dataloop

  Fields:
+ count - Number of elements
- dataloop - dataloop that this dataloop consists of

  Module:
  Datatype
  S*/
typedef struct {
    int     count;
    struct MPID_Dataloop_st *dataloop;
} MPID_Dataloop_contig;

/*S
  MPID_Dataloop_vector - Description of a vector or strided dataloop

  Fields:
+ count - Number of elements
. blocksize - Number of dataloops in each element
. stride - Stride (in bytes) between each block
- dataloop - Dataloop of each element

  Module:
  Datatype
  S*/
typedef struct { 
    int     count;
    struct MPID_Dataloop_st *dataloop;
    int     blocksize;
    int     stride;
} MPID_Dataloop_vector;

/*S
  MPID_Dataloop_blockindexed - Description of a block-indexed dataloop

  Fields:
+ count - Number of blocks
. blocksize - Number of elements in each block
. offset - Array of offsets (in bytes) to each block
- dataloop - Dataloop of each element

  Module:
  Datatype

  S*/
typedef struct {
    int      count;
    struct MPID_Dataloop_st *dataloop;
    int      blocksize;
    MPI_Aint *offset;
} MPID_Dataloop_blockindexed;

/*S
  MPID_Dataloop_indexed - Description of an indexed dataloop

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in bytes) to each block
- dataloop - Dataloop of each element

  Module:
  Datatype

  S*/
typedef struct {
    int      count;
    struct MPID_Dataloop_st *dataloop;
    int      *blocksize;
    MPI_Aint *offset;
} MPID_Dataloop_indexed;

/*S
  MPID_Dataloop_struct - Description of a structure dataloop

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in bytes) to each block
- dataloop - Array of dataloops describing the elements of each block

  Module:
  Datatype

  S*/
typedef struct {
    int      count;
    struct MPID_Dataloop_st *dataloop;
    int      *blocksize;
    MPI_Aint *offset;
} MPID_Dataloop_struct;

/* In many cases, we need the count and the next dataloop item. This
   common structure gives a quick access to both.  Note that all other 
   structures must use the same ordering of elements.
   Question: should we put the pointer first in case 
   sizeof(pointer)>sizeof(int) ? 
*/
typedef struct {
    int      count;
    struct MPID_Dataloop_st *dataloop;
} MPID_Dataloop_common;

/*S
  MPID_Dataloop - Description of the structure used to hold a dataloop
  description

  Fields:
+  kind - Describes the type of the dataloop.  This is divided into three
   separate bit fields\:
.vb
     Dataloop type (e.g., MPID_CONTIG etc.).  3 bits
     IsFinal (a "leaf" dataloop; see text) 1 bit
     Element Size (units for fields.) 2 bits
        Element size has 4 values
        0   - Elements are in units of bytes
        1   - Elements are in units of 2 bytes
        2   - Elements are in units of 4 bytes
        3   - Elements are in units of 8 bytes
.ve
  The dataloop type is one of 'MPID_CONTIG', 'MPID_DTYPE_VECTOR', 
  'MPID_BLOCKINDEXED', 'MPID_INDEXED', or 'MPID_STRUCT'.  
. loop_parms - A union containing the 5 dataloop structures, e.g., 
  'MPID_Dataloop_contig', 'MPID_Dataloop_vector', etc.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five dataloop structure.
. extent - The extent of the dataloop
- handle     - handle for the corresponding 'MPI_Datatype'.

  Module:
  Datatype

  S*/
typedef struct MPID_Dataloop_st { 
    int kind;                  /* Contains both the loop type 
				  (contig, vector, blockindexed, indexed,
				  or struct) and a bit that indicates 
				  whether the dataloop is a leaf type. */
    union {
	int                        count;
	MPID_Dataloop_contig       c_t;
	MPID_Dataloop_vector       v_t;
	MPID_Dataloop_blockindexed bi_t;
	MPID_Dataloop_indexed      i_t;
	MPID_Dataloop_struct       s_t;
	MPID_Dataloop_common       cm_t;
    } loop_params;
    MPI_Aint extent;
    int handle;                       /* Having the id here allows us to find the
				     full datatype structure from the 
				     Dataloop description */
} MPID_Dataloop;

#define DATALOOP_FINAL_MASK 0x00000008
#define DATALOOP_KIND_MASK  0x00000007
#define DATALOOP_ELMSIZE_SHIFT 4
#define MPID_DTYPE_CONTIG 0x1
#define MPID_DTYPE_VECTOR 0x2
#define MPID_DTYPE_BLOCKINDEXED 0x3
#define MPID_DTYPE_INDEXED 0x4
#define MPID_DTYPE_STRUCT 0x5
#define MPID_MAX_DATATYPE_DEPTH 8
/*S
  MPID_Dataloop_stackelm - Structure for an element of the stack used
  to process dataloops

  Fields:
+ curcount - Current loop count value (between 0 and 
             loopinfo.loop_params.count-1) 
. curoffset - Offset for relative offsets in dataloops 
- loopinfo  - Loop-based description of the dataloop

S*/
typedef struct {
    int           curcount;
    MPI_Aint      curoffset;
    MPID_Dataloop loopinfo;
} MPID_Dataloop_stackelm;

/* The max datatype depth is the maximum depth of the stack used to 
   evaluate datatypes.  It represents the length of the chain of 
   datatype dependencies.  Defining this and testing when a datatype
   is created removes a test for the datatype evaluation loop. */
#define MPID_MAX_DATATYPE_DEPTH 8


/*S
  MPID_Segment - Description of the Segment datatype

  Notes:
  This has no corresponding MPI datatype.

  The dataloop stack works as follows
.vb
  cur_sp=valid_sp=0;
  stackelm.loopinfo[cur_sp].cur_count = 0;
  stackelm.loopinfo[cur_sp].loopinfo = datatype->loopinfo;
  while (cur_sp >= 0) {
     if stackelm[cur_sp].loopinfo.kind is final then
        // final means simple, consisting of basic datatypes, such 
        // as a vector datatype made up of bytes or doubles)
        process datatype (this uses loopinfo.kind to pick the correct
            code fragments; we may also include some alignment tests
            so that longer word moves may be used for short (e.g.,
            one or two word) blocks).
        cur_sp--;
     else if stackelm[cur_sp].cur_count == stackelm[cur_sp].loopinfo.cm_t.count
         then {
         // We are done with the datatype.
         cur_sp--;
         }
     else {
        // need to push a datatype.  Two cases: struct or other
        if (stack_elm[cur_sp].loopinfo.kind == struct_type) {
           stack_elm[cur_sp+1].loopinfo = 
           stack_elm[cur_sp].loopinfo.s_t.dataloop[stackelm[cur_sp].cur_count];
           }
        else {
           if (valid_sp <= cur_sp) {
               stack_elm[cur_sp+1].loopinfo = 
               stack_elm[cur_sp].loopinfo.cm_t.dataloop;
               valid_sp = cur_sp + 1;
           }
        }
        stack_elm[cur_sp].cur_count++;
        cur_sp ++;
        stack_elm[cur_sp].cur_count = 0;
     }
  }
.ve 
  This may look a little bit messy (and not all of the code is here), but it 
  shows how 'valid_sp' is used to 
  avoid recopying the information on a datatype in the non-struct case.
  For example, a vector of vector has the datatype description read
  only once, not once for each count of the outer vector.

  Module:
  Segment

  Questions:
  Should this have an id for allocation and similarity purposes?
  S*/
typedef struct MPID_Segment_st { 
    void *ptr; /* pointer to datatype buffer */
    int stream_off; /* next offset into data stream resulting from datatype
		     * processing.  in other words, how many bytes have
		     * we created/used by parsing so far?  that amount + 1.
		     */
    MPID_Dataloop_stackelm loopinfo[MPID_MAX_DATATYPE_DEPTH];
    int  cur_sp;   /* Current stack pointer when using loopinfo */
    int  valid_sp; /* maximum valid stack pointer.  This is used to 
                      maintain information on the stack after it has
                      been placed there by following the datatype field
                      in a MPID_Dataloop_st for any type except struct */

    MPID_Dataloop builtin_loop; /* used for both predefined types (which
				 * won't have a loop already) and for 
				 * situations where a count is passed in 
				 * and we need to create a contig loop
				 * to handle it
				 */
    /* other, device-specific information */
    /* use these two bogus members for contiguous datatypes until the real code is available */
    void *bogus_user_buffer;
    int bogus_length;
    int bogus_dtype_extent;
} MPID_Segment;

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#define MPID_VECTOR         WSABUF
#define MPID_VECTOR_LEN     len
#define MPID_VECTOR_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_VECTOR         struct iovec
#define MPID_VECTOR_LEN     iov_len
#define MPID_VECTOR_BUF     iov_base
#endif
#define MPID_VECTOR_LIMIT   16

int MPID_Segment_init(void *buf, int count, MPI_Datatype handle, MPID_Segment *segp);
void MPID_Segment_pack(MPID_Segment *segp, int first, int *lastp, void *pack_buffer);
void MPID_Segment_unpack(MPID_Segment *segp, int first, int *lastp, const void *unpack_buffer);
void MPID_Segment_pack_vector(MPID_Segment *segp, int first, int *lastp, MPID_VECTOR *vector, int *lengthp);
void MPID_Segment_unpack_vector(MPID_Segment *segp, int first, int *lastp, MPID_VECTOR *vector, int *lengthp);

void MPIR_Segment_pack( MPID_Dataloop *loopinfo, char * restrict src_buf, char * restrict dest_buf );

#endif
