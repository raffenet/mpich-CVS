
/*
 * Each of the MPI datatypes can be mapped into one of 5 very simple
 * loops.  This loop has the following parameters:
 *    count  blocksize[] offset[]/stride datatype[]
 * where each [] indicates that a field may be *either* an array or a scalar.
 * For each such type, we define a struct that describes these parameters
 */

/*S
  MPID_Datatype_contig - Description of a contiguous datatype

  Fields:
+ count - Number of elements
- datatype - datatype that this datatype consists of

  Module:
  Datatype
  S*/
typedef struct {
    int count;
    struct dataloop_ *datatype;
} MPID_Datatype_contig;

/*S
  MPID_Datatype_vector - Description of a vector or strided datatype

  Fields:
+ count - Number of elements
. blocksize - Number of datatypes in each element
. stride - Stride (in bytes) between each block
- datatype - Datatype of each element

  Module:
  Datatype
  S*/
typedef struct { 
    int count;
    struct dataloop_ *datatype;
    int blocksize;
    int stride;
} MPID_Datatype_vector;

/*S
  MPID_Datatype_blockindexed - Description of a block-indexed datatype

  Fields:
+ count - Number of blocks
. blocksize - Number of elements in each block
. offset - Array of offsets (in bytes) to each block
- datatype - Datatype of each element

  Module:
  Datatype

  S*/
typedef struct {
    int count;
    struct dataloop_ *datatype;
    int blocksize;
    MPI_Aint *offset;
} MPID_Datatype_blockindexed;

/*S
  MPID_Datatype_indexed - Description of an indexed datatype

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in bytes) to each block
- datatype - Datatype of each element

  Module:
  Datatype

  S*/
typedef struct {
    int count;
    struct dataloop_ *datatype;
    int *blocksize;
    MPI_Aint *offset;
} MPID_Datatype_indexed;

/*S
  MPID_Datatype_struct - Description of a structure datatype

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in bytes) to each block
- datatype - Array of datatypes describing the elements of each block

  Module:
  Datatype

  S*/
typedef struct {
    int count;
    struct dataloop_ *datatype;
    int *blocksize;
    int *offset;
} MPID_Datatype_struct;

/* In many cases, we need the count and the next dataloop item. This
   common structure gives a quick access to both.  Note that all other 
   structures must use the same ordering of elements.
   Question: should we put the pointer first in case 
   sizeof(pointer)>sizeof(int) ? 
*/
typedef struct {
    int count;
    struct dataloop_ *datatype;
} MPID_Datatype_common;

/*S
  MPID_Dataloop - Description of the structure used to hold a datatype
  description

  Fields:
+  kind - Indicates what type of datatype.  May have the value
  'MPID_Contig', 'MPID_Vector', 'MPID_BlockIndexed', 'MPID_Indexed', or
  'MPID_Struct'.  
. loop_parms - A union containing the 5 datatype structures, e.g., 
  'MPID_Datatype_contig', 'MPID_Datatype_vector', etc.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five datatype structure.
. extent - The extent of the datatype
- id     - id for the corresponding 'MPI_Datatype'.

  Module:
  Datatype

  S*/
typedef struct MPID_Dataloop_st { 
    int kind;                  /* Contains both the loop type 
				  (contig, vector, blockindexed, indexed,
				  or struct) and a bit that indicates 
				  whether the datatype is a leaf type. */
    union {
	int                        count;
	MPID_Datatype_contig       c_t;
	MPID_Datatype_vector       v_t;
	MPID_Datatype_blockindexed bi_t;
	MPID_Datatype_indexed      i_t;
	MPID_Datatype_struct       s_t;
	MPID_Datatype_common       cm_t;
    } loop_params;
    MPI_Aint extent;
    int id;                       /* Having the id here allows us to find the
				     full datatype structure from the 
				     Dataloop description */
} MPID_Dataloop;

#define DATALOOP_FINAL_MASK 0x00000008
#define MPID_CONTIG 0x1
#define MPID_VECTOR 0x2
#define MPID_BLOCKINDEXED 0x3
#define MPID_INDEXED 0x4
#define MPID_STRUCT 0x5
#define MPID_MAX_DATATYPE_DEPTH 8

/*S
  MPID_Dataloop_stackelm - Structure for an element of the stack used
  to process datatypes

  Fields:
+ curcount - Current loop count value (between 0 and 
             loopinfo.loop_params.count-1) 
. curoffset - Offset for relative offsets in datatypes 
- loopinfo  - Loop-based description of the datatype

S*/
typedef struct {
    int           curcount;
    MPI_Aint      curoffset;
    MPID_Dataloop loopinfo;
} MPID_Dataloop_stackelm;

